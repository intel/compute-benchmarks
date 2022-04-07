/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/create_command_list_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const CreateCommandListImmediateArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties);
    Timer timer;

    // Warmup
    std::vector<ze_command_list_handle_t> commandLists(arguments.cmdListCount);
    ze_command_queue_desc_t commandQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    ze_command_list_handle_t commandList;
    for (auto i = 0u; i < arguments.cmdListCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));
        EXPECT_NE(nullptr, commandList);
        commandLists[i] = commandList;
    }
    for (auto i = 0u; i < arguments.cmdListCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandLists[i]));
        commandLists[i] = nullptr;
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        for (auto i = 0u; i < arguments.cmdListCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));
            commandLists[i] = commandList;
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);

        for (auto i = 0u; i < arguments.cmdListCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandLists[i]));
            commandLists[i] = nullptr;
        }
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CreateCommandListImmediate> registerTestCase(run, Api::L0);
