/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/mutable_command_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_update_mutable_commands.h"

#include <gtest/gtest.h>

using namespace L0;

namespace {

constexpr ze_mutable_command_exp_flags_t supportedFlags =
    ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
    ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;

struct MutationValues {
    int kernelArgument;
    ze_group_count_t groupCount;
    uint32_t groupSizeX;
    uint32_t globalOffsetX;
};

struct MutationChain {
    ze_mutable_kernel_argument_exp_desc_t kernelArgDesc{};
    ze_mutable_group_count_exp_desc_t groupCountDesc{};
    ze_mutable_group_size_exp_desc_t groupSizeDesc{};
    ze_mutable_global_offset_exp_desc_t globalOffsetDesc{};
    ze_mutable_commands_exp_desc_t mutableCommandsDesc{};
};

void buildMutationChain(MutationChain &chain, const MutationValues &values, uint64_t commandId, ze_mutable_command_exp_flags_t mutationFlags) {
    const void *chainHead = nullptr;
    if (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS) {
        chain.kernelArgDesc = {ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, chainHead,
                               commandId, 0u, sizeof(values.kernelArgument), &values.kernelArgument};
        chainHead = &chain.kernelArgDesc;
    }
    if (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT) {
        chain.groupCountDesc = {ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC, chainHead,
                                commandId, &values.groupCount};
        chainHead = &chain.groupCountDesc;
    }
    if (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE) {
        chain.groupSizeDesc = {ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC, chainHead,
                               commandId, values.groupSizeX, 1u, 1u};
        chainHead = &chain.groupSizeDesc;
    }
    if (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET) {
        chain.globalOffsetDesc = {ZE_STRUCTURE_TYPE_MUTABLE_GLOBAL_OFFSET_EXP_DESC, chainHead,
                                  commandId, values.globalOffsetX, 0u, 0u};
        chainHead = &chain.globalOffsetDesc;
    }
    chain.mutableCommandsDesc = {ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC, chainHead, 0u};
}

} // namespace

static TestResult run(const CommandListUpdateMutableCommandsArguments &arguments, Statistics &statistics) {
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

    ze_kernel_handle_t kernel = nullptr;
    ze_module_handle_t module = nullptr;
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }
    const MutationValues appendedValues{1, {1u, 1u, 1u}, 1u, 0u};
    const MutationValues mutatedValues{2, {2u, 1u, 1u}, 2u, 1u};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, appendedValues.groupSizeX, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(appendedValues.kernelArgument), &appendedValues.kernelArgument));

    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};

    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(mutableCmdList, &commandIdDesc, &commandId));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernel,
                                                             &appendedValues.groupCount, nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    MutationChain chainA;
    MutationChain chainB;
    buildMutationChain(chainA, mutatedValues, commandId, mutationFlags);
    buildMutationChain(chainB, appendedValues, commandId, mutationFlags);

    for (auto j = 0u; j < arguments.iterations; j++) {
        const ze_mutable_commands_exp_desc_t &mutableCommandsDesc = (j & 1u) ? chainB.mutableCommandsDesc : chainA.mutableCommandsDesc;

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandsExp(mutableCmdList, &mutableCommandsDesc));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    if (kernel) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    }
    if (module) {
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    }
    if (mutableCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mutableCmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListUpdateMutableCommands> registerTestCase(run, Api::L0);
