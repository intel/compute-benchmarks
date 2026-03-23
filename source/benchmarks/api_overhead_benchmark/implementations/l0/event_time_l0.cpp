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

#include "definitions/event_time.h"

#include <gtest/gtest.h>

static TestResult run(const EventTimeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero levelzero{extensionProperties};
    Timer timer;

    // prepare descriptor for event creation
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    ze_event_counter_based_flags_t cbFlags = 0;
    ze_event_scope_flags_t cbSignalScope = 0;
    ze_event_scope_flags_t cbWaitScope = 0;
    if (arguments.counterBasedEvents) {
        if (!levelzero.isCounterBasedEventsSupported()) {
            return TestResult::DeviceNotCapable;
        }

        cbFlags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE |
                  (arguments.hostVisible ? ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE : 0) |
                  (arguments.useProfiling ? ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP : 0);
    }
    if (arguments.signalScope == EventScope::scopeSubDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
        cbSignalScope = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.signalScope == EventScope::scopeDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        cbSignalScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.signalScope == EventScope::scopeHost) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        cbSignalScope = ZE_EVENT_SCOPE_FLAG_HOST;
    }
    if (arguments.waitScope == EventScope::scopeSubDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
        cbWaitScope = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.waitScope == EventScope::scopeDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
        cbWaitScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.waitScope == EventScope::scopeHost) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        cbWaitScope = ZE_EVENT_SCOPE_FLAG_HOST;
    }

    std::vector<ze_event_handle_t> events(arguments.eventCount);

    ze_event_pool_handle_t eventPool{}; // only for non-CBE
    if (!arguments.counterBasedEvents) {
        // Create event if necessary
        ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, arguments.eventCount};
        auto eventPoolFlags = arguments.hostVisible * ZE_EVENT_POOL_FLAG_HOST_VISIBLE | arguments.useProfiling * ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        eventPoolDesc.flags = eventPoolFlags;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            if (!arguments.counterBasedEvents) {
                eventDesc.index = j;
                ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
            } else {
                ze_event_counter_based_desc_t cbDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, .pNext = nullptr, .flags = cbFlags, .signal = cbSignalScope, .wait = cbWaitScope};
                ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &cbDesc, &events[j]));
            }
        }
        timer.measureEnd();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    if (!arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventTime> registerTestCase(run, Api::L0);
