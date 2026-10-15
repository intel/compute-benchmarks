/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/mutable_command_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_get_next_command_id.h"

#include <gtest/gtest.h>

using namespace L0;

namespace {

constexpr ze_mutable_command_exp_flags_t supportedFlags =
    ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET |
    ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT |
    ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS;

} // namespace

static TestResult run(const CommandListGetNextCommandIdArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    if (!levelzero.isMclExtensionAvailable(1, 0)) {
        return TestResult::DeviceNotCapable;
    }

    const MutableCommandFlag requestedFlag = static_cast<MutableCommandFlag>(arguments.mutableCommandFlag);
    const ze_mutable_command_exp_flags_t mutationFlags = MutableCommandHelper::toMutableCommandFlags(requestedFlag, supportedFlags);
    if (mutationFlags == 0) {
        return TestResult::InvalidArgs;
    }

    const ze_mutable_command_exp_flags_t deviceFlags = levelzero.getDeviceMclProperties().mutableCommandFlags;
    if ((deviceFlags & mutationFlags) != mutationFlags) {
        return TestResult::DeviceNotCapable;
    }

    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};
    uint64_t commandId = 0;

    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(
            mutableCmdList, &commandIdDesc, &commandId));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(mutableCmdList));
    }

    if (mutableCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mutableCmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListGetNextCommandId> registerTestCase(run, Api::L0);