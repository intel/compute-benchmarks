/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (arguments.counterBasedEvents && !arguments.inOrder) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(
        arguments.counterBasedEvents);
    LevelZero levelzero(extensionProperties);

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    const size_t gws = 1024u;
    const size_t lws = 64u;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_eat_time.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * 8);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    // Create command list and append kernel
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    if (arguments.inOrder) {
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }

    // Create events for profiling
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_NON_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
    if (arguments.hostVisible) {
        counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
        counterBasedEventDesc.signalScope |= ZE_EVENT_SCOPE_FLAG_HOST;
    }

    ze_event_pool_flags_t flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    if (arguments.hostVisible) {
        flags |= ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    }

    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, flags, static_cast<uint32_t>(arguments.kernelCount)};

    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool = nullptr;
    if (!arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));
    }

    std::vector<ze_event_handle_t> profilingEvents(arguments.kernelCount);
    for (auto i = 0u; i < arguments.kernelCount; i++) {
        if (arguments.counterBasedEvents) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &profilingEvents[i]));
        } else {
            ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, 0, 0};
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &profilingEvents[i]));
        }
    }

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, profilingEvents[0], 0, nullptr));
    for (auto j = 1u; j < arguments.kernelCount; j++) {
        if (arguments.barrier) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0u, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, profilingEvents[j], 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, profilingEvents[j], 1, &profilingEvents[j - 1]));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        // Benchmark
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto j = 1u; j < arguments.kernelCount; j++) {
            ze_kernel_timestamp_result_t earlierKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[j - 1], &earlierKernelTimestamp));
            ze_kernel_timestamp_result_t laterKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[j], &laterKernelTimestamp));

            switchTime += std::chrono::nanoseconds((laterKernelTimestamp.global.kernelStart - earlierKernelTimestamp.global.kernelEnd) * timerResolution);
        }
        statistics.pushValue(switchTime / (arguments.kernelCount - 1), typeSelector.getUnit(), typeSelector.getType());
        if (!arguments.counterBasedEvents) {
            for (auto j = 0u; j < arguments.kernelCount; j++) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvents[j]));
            }
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (auto &hEvent : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    }
    if (!arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSwitchLatency> registerTestCase(run, Api::L0);
