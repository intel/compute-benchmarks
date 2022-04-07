/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/event_ctxt_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const EventCtxtSwitchLatencyArguments &arguments, Statistics &statistics) {
    QueueProperties queuePropertiesFirst = QueueProperties::create().setForceEngine(arguments.firstEngine);
    LevelZero levelzero(queuePropertiesFirst, ContextProperties::create());
    auto queueDescFirst = levelzero.commandQueueDesc;
    auto cmdQueueFirst = levelzero.commandQueue;
    auto ordinalFirst = queueDescFirst.ordinal;

    const uint64_t timerResolution = levelzero.getTimerResoultion(levelzero.device);

    QueueProperties queuePropertiesSecond = QueueProperties::create().setForceEngine(arguments.secondEngine);
    auto queueDescSecond = levelzero.createQueue(queuePropertiesSecond);
    auto cmdQueueSecond = queueDescSecond.queue;
    auto ordinalSecond = queueDescSecond.family.ordinal;

    // Create buffer
    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    const auto bufferSize = sizeof(uint64_t) * 2;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize))
    uint64_t *beginTimestamp = static_cast<uint64_t *>(buffer);
    uint64_t *endTimestamp = beginTimestamp + 1;

    // Create events
    uint32_t eventsCount = static_cast<uint32_t>(arguments.measuredCommands);
    const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, eventsCount};
    ze_event_pool_handle_t eventPool{};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_SUBDEVICE, ZE_EVENT_SCOPE_FLAG_SUBDEVICE};
    std::vector<ze_event_handle_t> events(eventsCount);
    ZE_RESULT_SUCCESS_OR_ERROR(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    for (uint32_t eventIndex = 0u; eventIndex < eventsCount; eventIndex++) {
        eventDesc.index = eventIndex;
        auto event = &events[eventIndex];
        ZE_RESULT_SUCCESS_OR_ERROR(zeEventCreate(eventPool, &eventDesc, event));
    }

    // Create command lists
    ze_command_list_desc_t cmdListDescFirst{};
    cmdListDescFirst.commandQueueGroupOrdinal = ordinalFirst;
    ze_command_list_handle_t cmdListFirst{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDescFirst, &cmdListFirst));

    ze_command_list_desc_t cmdListDescSecond{};
    cmdListDescSecond.commandQueueGroupOrdinal = ordinalSecond;
    ze_command_list_handle_t cmdListSecond{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDescSecond, &cmdListSecond));

    ze_command_list_desc_t cmdListDescReset{};
    cmdListDescReset.commandQueueGroupOrdinal = ordinalFirst;
    ze_command_list_handle_t cmdListReset{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDescReset, &cmdListReset));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdListFirst, beginTimestamp, nullptr, 0, nullptr));
    for (auto commandIndex = 0u; commandIndex < arguments.measuredCommands; commandIndex++) {
        if (commandIndex % 2 == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdListFirst, events[commandIndex]));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(cmdListFirst, 1, &events[commandIndex + 1]));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(cmdListSecond, 1, &events[commandIndex - 1]));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdListSecond, events[commandIndex]));
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendEventReset(cmdListReset, events[commandIndex]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdListFirst, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdListFirst));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdListSecond));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdListReset));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueFirst, 1, &cmdListFirst, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueSecond, 1, &cmdListSecond, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cmdQueueFirst, std::numeric_limits<uint64_t>::max()));

    // reset all events before next round
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueFirst, 1, &cmdListReset, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cmdQueueFirst, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueFirst, 1, &cmdListFirst, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueSecond, 1, &cmdListSecond, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cmdQueueFirst, std::numeric_limits<uint64_t>::max()));

        // reset all events before next round
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueueFirst, 1, &cmdListReset, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cmdQueueFirst, std::numeric_limits<uint64_t>::max()));

        auto commandTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        commandTime *= timerResolution;
        commandTime /= arguments.measuredCommands;
        statistics.pushValue(commandTime, MeasurementUnit::Microseconds, MeasurementType::Gpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdListFirst));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdListSecond));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdListReset));
    for (auto &event : events) {
        EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventCtxtSwitchLatency> registerTestCase(run, Api::L0);
