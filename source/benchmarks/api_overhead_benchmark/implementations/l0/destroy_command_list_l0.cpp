/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/destroy_command_list.h"

#include <gtest/gtest.h>

static TestResult run(const DestroyCommandListArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    std::vector<ze_command_list_handle_t> commandLists(arguments.cmdListCount);
    ze_command_list_desc_t commandListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    commandListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t commandList;

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {

        for (auto i = 0u; i < arguments.cmdListCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &commandListDesc, &commandList));
            commandLists[i] = commandList;
        }

        timer.measureStart();
        for (auto i = 0u; i < arguments.cmdListCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandLists[i]));
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DestroyCommandList> registerTestCase(run, Api::L0);
