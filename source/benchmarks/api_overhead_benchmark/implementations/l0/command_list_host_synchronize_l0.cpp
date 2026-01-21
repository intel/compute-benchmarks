/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_host_synchronize.h"

#include <gtest/gtest.h>

static TestResult run(const CommandListHostSynchronizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useBarrierBeforeSync) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Create an immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    auto runBenchmark = [&]() {
        if (arguments.useBarrierBeforeSync) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, event, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        return TestResult::Success;
    };

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        auto status = runBenchmark();
        if (status != TestResult::Success) {
            return status;
        }
        timer.measureEnd();
        if (arguments.useBarrierBeforeSync) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    if (arguments.useBarrierBeforeSync) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListHostSynchronize> registerTestCase(run, Api::L0);