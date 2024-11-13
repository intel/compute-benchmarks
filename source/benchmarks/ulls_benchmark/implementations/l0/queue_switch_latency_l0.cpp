/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/queue_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const QueueSwitchArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;

    const size_t lws = 32u;
    const size_t gws = lws;

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
    int kernelOperationsCount = static_cast<int>(arguments.kernelTime);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    // Create command list and append kernel
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    ze_command_list_handle_t cmdList1{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList1));
    ze_command_list_handle_t cmdList2{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList2));

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    zex_counter_based_event_desc_t counterBasedEventDesc{ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_NON_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;

    std::vector<ze_event_handle_t> profilingEvents(arguments.switchCount + 1);
    for (auto eventId = 0u; eventId < arguments.switchCount + 1; eventId++) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &profilingEvents[eventId]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList1, kernel, &groupCount, profilingEvents[0], 0, nullptr));
    for (auto switchIdentifier = 0u; switchIdentifier < arguments.switchCount; switchIdentifier++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel((switchIdentifier % 2) == 1 ? cmdList2 : cmdList1, kernel, &groupCount, profilingEvents[switchIdentifier + 1], 1, &profilingEvents[switchIdentifier]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList2));

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

    ze_command_queue_handle_t queue1;
    ze_command_queue_handle_t queue2;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context, levelzero.device, &commandQueueDesc, &queue1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context, levelzero.device, &commandQueueDesc, &queue2));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue1, 1, &cmdList1, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue2, 1, &cmdList2, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue1, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue2, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue1, 1, &cmdList1, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue2, 1, &cmdList2, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue1, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue2, std::numeric_limits<uint64_t>::max()));
        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto switchId = 0u; switchId < arguments.switchCount; switchId++) {
            ze_kernel_timestamp_result_t earlierKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[switchId], &earlierKernelTimestamp));
            ze_kernel_timestamp_result_t laterKernelTimestamp;
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[switchId + 1], &laterKernelTimestamp));

            switchTime += std::chrono::nanoseconds((laterKernelTimestamp.global.kernelStart - earlierKernelTimestamp.global.kernelEnd) * timerResolution);
        }
        statistics.pushValue(switchTime / (arguments.switchCount), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto &hEvent : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList2));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(queue1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(queue2));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueueSwitch> registerTestCase(run, Api::L0);
