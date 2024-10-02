/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/reset_command_list.h"

#include <gtest/gtest.h>

static TestResult run(const ResetCommandListArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.copyOnly).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (nullptr == levelzero.commandQueue) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Create buffers
    void *source{}, *destination{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    if (isUsmMemoryType(arguments.sourcePlacement)) {
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, source, arguments.size, BufferContents::Zeros, false));
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &destination));

    // Create command list
    ze_command_list_desc_t commandListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    commandListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t commandList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &commandListDesc, &commandList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(commandList, destination, source, arguments.size, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(commandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &commandList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(commandList));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(commandList, destination, source, arguments.size, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(commandList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &commandList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(commandList));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ResetCommandList> registerTestCase(run, Api::L0);
