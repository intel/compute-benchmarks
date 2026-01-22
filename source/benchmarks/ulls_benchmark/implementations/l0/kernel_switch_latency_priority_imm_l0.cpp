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

#include "definitions/kernel_switch_latency_priority_imm.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchPriorityImmArguments &arguments, Statistics &statistics) {
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
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    commandQueueDesc.priority = QueueProperties::create().setPriority(arguments.measuredQueuePriority).priority;
    if (arguments.useIoq) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t mainCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &mainCmdList));

    commandQueueDesc.priority = QueueProperties::create().setPriority(arguments.secondaryQueuePriority).priority;
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t secondCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &secondCmdList));

    // Create events for profiling
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, static_cast<uint32_t>(arguments.kernelCount)};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool = nullptr;
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));
    }
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
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
    auto launchIteration = [&]() {
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
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(mainCmdList, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(secondCmdList, std::numeric_limits<uint64_t>::max()));
        return TestResult::Success;
    };

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        // Benchmark
        auto ret = launchIteration();
        if (ret != TestResult::Success) {
            return ret;
        }

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

static RegisterTestCaseImplementation<KernelSwitchPriorityImm> registerTestCase(run, Api::L0);
