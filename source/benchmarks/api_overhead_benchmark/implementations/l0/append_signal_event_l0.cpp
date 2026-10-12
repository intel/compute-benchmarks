/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_signal_event.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const AppendSignalEventArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (arguments.counterBasedEvents && !levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    Timer timer;

    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(arguments.appendCount);
    if (arguments.counterBasedEvents) {
        ze_event_counter_based_desc_t eventDesc = defaultIntelCounterBasedEventDesc;
        eventDesc.flags = ZE_EVENT_COUNTER_BASED_FLAG_NON_IMMEDIATE;
        if (arguments.timestampEvents) {
            eventDesc.flags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        }
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &events[j]));
        }
    } else {
        const ze_event_pool_flags_t eventPoolFlags = arguments.timestampEvents ? ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP : 0;
        const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, eventPoolFlags, static_cast<uint32_t>(arguments.appendCount)};
        ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdList, events[j]));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.appendCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    for (auto j = 0u; j < arguments.appendCount; ++j) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
    }
    if (eventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendSignalEvent> registerTestCase(run, Api::L0);
