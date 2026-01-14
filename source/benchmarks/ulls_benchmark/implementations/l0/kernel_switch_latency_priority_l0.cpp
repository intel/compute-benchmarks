/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_switch_latency_priority.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchPriorityArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero levelzero(extensionProperties);

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    const size_t lws = 1024u;
    const size_t gws = lws * 64;

    // Create kernel
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_eat_time.cl", "eat_time", &kernel, &module);
    if (kernelLoadRes != TestResult::Success)
        return kernelLoadRes;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    // Create command lists
    auto queueDesc = levelzero.createQueue(QueueProperties::create().setPriority(arguments.measuredQueuePriority));
    auto secondQueueDesc = levelzero.createQueue(QueueProperties::create().setPriority(arguments.secondaryQueuePriority));
    auto mainCmdQueue = queueDesc.queue;
    auto secondCmdQueue = secondQueueDesc.queue;

    ze_command_list_desc_t cmdListDesc{};
    if (arguments.useIoq) {
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }
    cmdListDesc.commandQueueGroupOrdinal = queueDesc.family.ordinal;
    ze_command_list_handle_t mainCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mainCmdList));

    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    cmdListDesc.commandQueueGroupOrdinal = secondQueueDesc.family.ordinal;
    ze_command_list_handle_t secondCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &secondCmdList));

    // Create events for profiling
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, static_cast<uint32_t>(arguments.kernelCount)};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool = nullptr;
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));
    }
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_NON_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
    counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
    counterBasedEventDesc.signalScope |= ZE_EVENT_SCOPE_FLAG_HOST;

    std::vector<ze_event_handle_t> profilingEvents(arguments.kernelCount);
    for (auto i = 0u; i < arguments.kernelCount; i++) {
        if (arguments.useIoq) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &profilingEvents[i]));
        } else {
            ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, 0, 0};
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &profilingEvents[i]));
        }
    }

    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mainCmdList, kernel, &groupCount, profilingEvents[0], 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(secondCmdList, kernel, &groupCount, nullptr, 0, nullptr));
    for (auto j = 1u; j < arguments.kernelCount; j++) {
        int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * 8);
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
        auto dependentEvent = !arguments.useIoq ? profilingEvents[j - 1] : nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mainCmdList, kernel, &groupCount, profilingEvents[j], dependentEvent ? 1 : 0, &dependentEvent));

        kernelOperationsCount *= 4;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(secondCmdList, kernel, &groupCount, nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mainCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(secondCmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(mainCmdQueue, 1, &mainCmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(secondCmdQueue, 1, &secondCmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(mainCmdQueue, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(secondCmdQueue, std::numeric_limits<uint64_t>::max()));
    if (!arguments.useIoq) {
        for (auto j = 0u; j < arguments.kernelCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvents[j]));
        }
    }

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        // Benchmark
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(mainCmdQueue, 1, &mainCmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(secondCmdQueue, 1, &secondCmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(mainCmdQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(secondCmdQueue, std::numeric_limits<uint64_t>::max()));

        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto j = 1u; j < arguments.kernelCount; j++) {
            ze_kernel_timestamp_result_t earlierKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[j - 1], &earlierKernelTimestamp));
            ze_kernel_timestamp_result_t laterKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[j], &laterKernelTimestamp));

            switchTime += std::chrono::nanoseconds((laterKernelTimestamp.global.kernelStart - earlierKernelTimestamp.global.kernelEnd) * timerResolution);
        }
        statistics.pushValue(switchTime / (arguments.kernelCount - 1), typeSelector.getUnit(), typeSelector.getType());

        if (!arguments.useIoq) {
            for (auto j = 0u; j < arguments.kernelCount; j++) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvents[j]));
            }
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mainCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(secondCmdList));
    for (auto &hEvent : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    }
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSwitchPriority> registerTestCase(run, Api::L0);
