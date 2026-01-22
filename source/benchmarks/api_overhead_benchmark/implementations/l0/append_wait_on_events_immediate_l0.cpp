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

#include "definitions/append_wait_on_events_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const AppendWaitOnEventsImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // create command list
    ze_command_queue_desc_t commandQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    if (arguments.useIoq) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t commandList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));

    // Create event
    ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 2u};
    auto eventPoolFlags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.flags = eventPoolFlags;
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    if (arguments.eventSignaled) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(event));
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(commandList, 1, &event));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // unlock GPU
    if (!arguments.eventSignaled) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(event));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendWaitOnEventsImmediate> registerTestCase(run, Api::L0);
