/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_mem_copy.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const AppendMemCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Allocate source and destination memory
    void *src{};
    void *dst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.src, levelzero, arguments.size, &src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dst, levelzero, arguments.size, &dst));

    // Create event if necessary
    const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, arguments.appendCount};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(arguments.appendCount);
    if (arguments.useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    // Create command list and warmup
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    for (auto j = 0u; j < arguments.appendCount; ++j) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dst, src, arguments.size, events[j], 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dst, src, arguments.size, events[j], 0, nullptr));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.appendCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    // Cleanup
    if (arguments.useEvent) {
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.src, levelzero, src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dst, levelzero, dst));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendMemCopy> registerTestCase(run, Api::L0);
