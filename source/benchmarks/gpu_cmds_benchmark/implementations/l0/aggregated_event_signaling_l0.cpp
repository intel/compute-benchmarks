/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/aggregated_event_signaling.h"

#include <gtest/gtest.h>

static TestResult run(const AggregatedEventSignalingArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    // Create buffers
    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *timestampBuffer = nullptr;
    const auto timestampBufferSize = sizeof(uint64_t) * 2;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, timestampBufferSize, 0, &timestampBuffer));
    uint64_t *beginTimestamp = static_cast<uint64_t *>(timestampBuffer);
    uint64_t *endTimestamp = beginTimestamp + 1;

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *aggregateStorage = nullptr;
    const auto aggregateStorageSize = sizeof(uint64_t);
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, aggregateStorageSize, alignof(uint64_t), levelzero.device, &aggregateStorage));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, aggregateStorage, aggregateStorageSize));

    // Create event
    ze_event_handle_t event = nullptr;
    uint32_t incrementValue = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetAggregatedCopyOffloadIncrementValue(levelzero.device, &incrementValue));
    ze_event_counter_based_external_aggregate_storage_desc_t aggregateStorageDesc{
        .stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_EXTERNAL_AGGREGATE_STORAGE_DESC,
        .deviceAddress = static_cast<uint64_t *>(aggregateStorage),
        .incrementValue = incrementValue,
        .completionValue = arguments.measuredCommands * incrementValue};

    ze_event_counter_based_desc_t eventDesc{
        .stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC,
        .pNext = &aggregateStorageDesc,
        .flags = ZE_EVENT_COUNTER_BASED_FLAG_NON_IMMEDIATE,
        .signal = ZE_EVENT_SCOPE_FLAG_DEVICE,
        .wait = ZE_EVENT_SCOPE_FLAG_DEVICE,
    };

    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &event));

    // Create command list
    ze_command_list_handle_t cmdList = nullptr;
    ze_command_list_desc_t cmdListDesc{
        .stype = ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
        .commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal,
        .flags = ZE_COMMAND_LIST_FLAG_IN_ORDER};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    // Record commands
    const uint64_t zero = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, aggregateStorage, &zero, sizeof(zero), aggregateStorageSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    for (auto commandIndex = 0u; commandIndex < arguments.measuredCommands; commandIndex++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdList, event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (size_t i = 0; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        auto commandTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        commandTime *= timerResolution;
        commandTime /= arguments.measuredCommands;
        statistics.pushValue(commandTime, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, aggregateStorage, aggregateStorageSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, aggregateStorage));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AggregatedEventSignaling> registerTestCase(run, Api::L0);
