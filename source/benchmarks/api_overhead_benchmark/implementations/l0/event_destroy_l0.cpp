/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/event_destroy.h"

#include <gtest/gtest.h>

static TestResult run(const EventDestroyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    if (arguments.counterBasedEvents) {
        extensionProperties.setCounterBasedCreateFunctions(true);
    }
    LevelZero levelzero{extensionProperties};
    Timer timer;

    // prepare descriptor for event creation
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    zex_counter_based_event_desc_t eventDescCBE{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    if (arguments.counterBasedEvents) {
        if (levelzero.counterBasedEventCreate2 == nullptr) {
            return TestResult::DeviceNotCapable;
        }

        eventDescCBE.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE |
                             (arguments.hostVisible ? ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE : 0) |
                             (arguments.useProfiling ? ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP : 0);
    }
    if (arguments.signalScope == EventScope::scopeSubDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
        eventDescCBE.signalScope = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.signalScope == EventScope::scopeDevice) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDescCBE.signalScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.signalScope == EventScope::scopeHost) {
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDescCBE.signalScope = ZE_EVENT_SCOPE_FLAG_HOST;
    }
    if (arguments.waitScope == EventScope::scopeSubDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
        eventDescCBE.waitScope = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    } else if (arguments.waitScope == EventScope::scopeDevice) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDescCBE.waitScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    } else if (arguments.waitScope == EventScope::scopeHost) {
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDescCBE.waitScope = ZE_EVENT_SCOPE_FLAG_HOST;
    }

    std::vector<ze_event_handle_t> events(arguments.eventCount);
    ze_event_pool_handle_t eventPool{};

    if (!arguments.counterBasedEvents) {
        ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, arguments.eventCount};
        auto eventPoolFlags = arguments.hostVisible * ZE_EVENT_POOL_FLAG_HOST_VISIBLE | arguments.useProfiling * ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        eventPoolDesc.flags = eventPoolFlags;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        for (auto j = 0u; j < arguments.eventCount; ++j) {
            if (arguments.counterBasedEvents) {
                // CBE is marked as completed so there is no need to signal it.
                ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &eventDescCBE, &events[j]));
            } else {
                eventDesc.index = j;
                ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(events[j]));
            }

            // Validate event is already completed
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(events[j]));
        }

        timer.measureStart();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    if (!arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventDestroy> registerTestCase(run, Api::L0);