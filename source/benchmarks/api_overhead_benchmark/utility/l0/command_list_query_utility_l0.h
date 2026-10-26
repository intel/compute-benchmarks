/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/l0/levelzero.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include <level_zero/ze_api.h>

template <typename Value, typename QueryValue, typename IsExpectedValue>
TestResult measureCommandListQuery(const TestCaseArgumentContainer &arguments, Statistics &statistics, bool useImmediate,
                                   const QueryValue &queryValue, const IsExpectedValue &isExpectedValue) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::LevelZero levelzero;
    Timer timer;

    ze_command_list_handle_t cmdList{};
    if (useImmediate) {
        ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
        commandQueueDesc.ordinal = levelzero.commandQueueDesc.ordinal;
        commandQueueDesc.index = levelzero.commandQueueDesc.index;
        commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    } else {
        ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
        cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    }

    // One call costs a few nanoseconds, less than the fences and clock reads of the timer.
    constexpr uint32_t implicitIterationCount = 100;
    bool valueMatches = true;
    for (auto i = 0u; i < arguments.iterations; ++i) {
        Value value{};

        timer.measureStart();
        for (auto j = 0u; j < implicitIterationCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(queryValue(cmdList, &value));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / implicitIterationCount, typeSelector.getUnit(), typeSelector.getType());

        valueMatches &= isExpectedValue(levelzero, value);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return valueMatches ? TestResult::Success : TestResult::VerificationFail;
}
