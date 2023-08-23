/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/event_time.h"

#include <gtest/gtest.h>

static TestResult run(const EventTimeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create event if necessary
    ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, arguments.eventCount};
    auto eventPoolFlags = arguments.hostVisible * ZE_EVENT_POOL_FLAG_HOST_VISIBLE | arguments.useProfiling * ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    eventPoolDesc.flags = eventPoolFlags;

    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    if (arguments.signalScope == EventScope::scopeSubDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.signalScope == EventScope::scopeDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.signalScope == EventScope::scopeHost) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    }

    if (arguments.waitScope == EventScope::scopeSubDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.signalScope == EventScope::scopeDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.signalScope == EventScope::scopeHost) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    }

    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(arguments.eventCount);
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));

    // warmup
    for (auto j = 0u; j < arguments.eventCount; ++j) {
        eventDesc.index = j;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
    }
    for (auto j = 0u; j < arguments.eventCount; ++j) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
        timer.measureEnd();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        statistics.pushValue(timer.get() / arguments.eventCount, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventTime> registerTestCase(run, Api::L0);
