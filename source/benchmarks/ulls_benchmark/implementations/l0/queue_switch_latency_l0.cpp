/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/queue_switch_latency.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const QueueSwitchArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;

    const size_t lws = static_cast<size_t>(arguments.workgroupSize);
    const size_t gws = lws * static_cast<size_t>(arguments.workgroupCount);

    // Create kernels
    ze_module_handle_t module{};
    ze_kernel_handle_t kernelA{};
    ze_kernel_handle_t kernelB{};
    auto kernelLoadResult = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_eat_time.cl", "eat_time", &kernelA, &module, nullptr);
    if (kernelLoadResult != TestResult::Success) {
        return kernelLoadResult;
    }

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time2";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernelB));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernelA, static_cast<uint32_t>(lws), 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernelB, static_cast<uint32_t>(lws), 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelTime);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernelA, 0, sizeof(int), &kernelOperationsCount));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernelB, 0, sizeof(int), &kernelOperationsCount));

    // Create command lists and append kernels
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    const size_t queueCount = static_cast<size_t>(arguments.queuesCount);
    std::vector<ze_command_list_handle_t> commandLists(queueCount);
    for (size_t queueId = 0; queueId < queueCount; queueId++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &commandLists[queueId]));
    }

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    eventPoolDesc.count = static_cast<uint32_t>(arguments.switchCount + 1);
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;

    std::vector<ze_event_handle_t> profilingEvents(arguments.switchCount + 1);
    for (auto eventId = 0u; eventId < arguments.switchCount + 1; eventId++) {
        eventDesc.index = eventId;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &profilingEvents[eventId]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandLists[0], kernelA, &groupCount, profilingEvents[0], 0, nullptr));
    for (auto switchId = 0u; switchId < arguments.switchCount; switchId++) {
        const auto queueId = static_cast<size_t>(switchId + 1u) % queueCount;
        const auto kernelId = switchId + 1u;

        ze_kernel_handle_t selectedKernel = kernelA;
        if (arguments.differentKernels && (kernelId % 2u)) {
            selectedKernel = kernelB;
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandLists[queueId], selectedKernel, &groupCount, profilingEvents[kernelId], 1, &profilingEvents[switchId]));
    }

    for (auto &commandList : commandLists) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(commandList));
    }

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

    std::vector<ze_command_queue_handle_t> queues(queueCount);
    for (size_t queueId = 0; queueId < queueCount; queueId++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context, levelzero.device, &commandQueueDesc, &queues[queueId]));
    }

    // Benchmark
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        for (size_t queueId = 0; queueId < queueCount; queueId++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queues[queueId], 1, &commandLists[queueId], nullptr));
        }
        for (auto &queue : queues) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max()));
        }
        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto switchId = 0u; switchId < arguments.switchCount; switchId++) {
            ze_kernel_timestamp_result_t earlierKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[switchId], &earlierKernelTimestamp));
            ze_kernel_timestamp_result_t laterKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[switchId + 1], &laterKernelTimestamp));

            switchTime += std::chrono::nanoseconds((laterKernelTimestamp.global.kernelStart - earlierKernelTimestamp.global.kernelEnd) * timerResolution);
        }
        statistics.pushValue(switchTime / (arguments.switchCount), typeSelector.getUnit(), typeSelector.getType());

        for (auto &hEvent : profilingEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(hEvent));
        }
    }

    for (auto &hEvent : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelA));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelB));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    for (auto &commandList : commandLists) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));
    }
    for (auto &queue : queues) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(queue));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueueSwitch> registerTestCase(run, Api::L0);
