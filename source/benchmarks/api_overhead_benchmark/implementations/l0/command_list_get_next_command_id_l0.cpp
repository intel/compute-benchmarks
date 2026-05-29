/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_get_next_command_id.h"

#include <gtest/gtest.h>

namespace {

// Returns 0 if the requested flag is not supported by this benchmark.
ze_mutable_command_exp_flags_t resolveSupportedFlags(MutableCommandFlag flag) {
    constexpr ze_mutable_command_exp_flags_t supportedUnion =
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET |
        ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT |
        ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS;

    switch (flag) {
    case MutableCommandFlag::All:
        return supportedUnion;
    case MutableCommandFlag::KernelArguments:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS;
    case MutableCommandFlag::GroupCount:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT;
    case MutableCommandFlag::GroupSize:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE;
    case MutableCommandFlag::GlobalOffset:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;
    case MutableCommandFlag::SignalEvent:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT;
    case MutableCommandFlag::WaitEvents:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS;
    case MutableCommandFlag::GraphArguments:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS;
    default:
        return 0;
    }
}

} // namespace

static TestResult run(const CommandListGetNextCommandIdArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    if (!levelzero.isMclExtensionAvailable(1, 0)) {
        return TestResult::DeviceNotCapable;
    }

    const MutableCommandFlag requestedFlag = static_cast<MutableCommandFlag>(arguments.mutableCommandFlag);
    const ze_mutable_command_exp_flags_t mutationFlags = resolveSupportedFlags(requestedFlag);
    if (mutationFlags == 0) {
        return TestResult::InvalidArgs;
    }

    if (!(levelzero.getDeviceMclProperties().mutableCommandFlags & mutationFlags)) {
        return TestResult::DeviceNotCapable;
    }

    // Create mutable command list
    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    // Create mutable command identifier descriptor
    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};
    uint64_t commandId = 0;

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(
        mutableCmdList, &commandIdDesc, &commandId));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(
            mutableCmdList, &commandIdDesc, &commandId));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    if (mutableCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mutableCmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListGetNextCommandId> registerTestCase(run, Api::L0);