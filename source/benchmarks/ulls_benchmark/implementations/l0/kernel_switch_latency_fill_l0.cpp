/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_switch_latency_fill.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchLatencyFillArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero levelzero(extensionProperties);

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    const size_t gws = arguments.fillSize / sizeof(uint32_t);
    const size_t lws = 256u;

    // Create kernel
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_fill_with_ones.cl", "fill_with_ones", &kernel, &module);
    if (kernelLoadRes != TestResult::Success)
        return kernelLoadRes;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    void *buffer;
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.fillSize * sizeof(uint32_t), &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    // Create events for profiling
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> profilingEvents(arguments.kernelCount);
    if (arguments.inOrder) {
        zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
        counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_NON_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
        counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
        counterBasedEventDesc.signalScope |= ZE_EVENT_SCOPE_FLAG_HOST;
        for (auto i = 0u; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &profilingEvents[i]));
        }
    } else {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        eventPoolDesc.count = static_cast<uint32_t>(arguments.kernelCount);
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        for (auto i = 0u; i < arguments.kernelCount; i++) {
            eventDesc.index = i;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &profilingEvents[i]));
        }
    }

    // Create command list and append kernel
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    if (arguments.inOrder) {
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, profilingEvents[0], 0, nullptr));
    for (auto j = 1u; j < arguments.kernelCount; j++) {
        auto waitListCount = arguments.inOrder ? 0 : 1;
        auto waitListEvent = arguments.inOrder ? nullptr : &profilingEvents[j - 1];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, profilingEvents[j], waitListCount, waitListEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    if (!arguments.inOrder) {
        for (auto &event : profilingEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
    }

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
        if (!arguments.inOrder) {
            for (auto &event : profilingEvents) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
            }
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (auto &event : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    if (!arguments.inOrder) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSwitchLatencyFill> registerTestCase(run, Api::L0);
