/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_update_mutable_commands.h"

#include <gtest/gtest.h>

namespace {

ze_mutable_command_exp_flags_t resolveSupportedFlags(MutableCommandFlag flag) {
    constexpr ze_mutable_command_exp_flags_t supportedUnion =
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET;

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
    default:
        return 0;
    }
}

} // namespace

static TestResult run(const CommandListUpdateMutableCommandsArguments &arguments, Statistics &statistics) {
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

    // Create module + kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module = nullptr;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    ze_kernel_handle_t kernel = nullptr;
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    // Initial kernel argument value (overwritten by mutations). eat_time takes one int.
    const int initialKernelOpsCount = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &initialKernelOpsCount));

    // Append a single launch so the obtained commandId references a real mutable command.
    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};
    ze_group_count_t initialGroupCount = {1u, 1u, 1u};

    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(mutableCmdList, &commandIdDesc, &commandId));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernel,
                                                             &initialGroupCount, nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    // Build chained pNext list of mutation descriptors
    const bool mutateKernelArgs = (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS) != 0;
    const bool mutateGroupCount = (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT) != 0;
    const bool mutateGroupSize = (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE) != 0;
    const bool mutateGlobalOffset = (mutationFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_GLOBAL_OFFSET) != 0;

    const int newKernelArgValue = 2;
    const ze_group_count_t newGroupCount = {1u, 1u, 1u};

    const void *chainHead = nullptr;
    ze_mutable_kernel_argument_exp_desc_t kernelArgDesc{};
    ze_mutable_group_count_exp_desc_t groupCountDesc{};
    ze_mutable_group_size_exp_desc_t groupSizeDesc{};
    ze_mutable_global_offset_exp_desc_t globalOffsetDesc{};

    if (mutateKernelArgs) {
        kernelArgDesc = ze_mutable_kernel_argument_exp_desc_t{
            ZE_STRUCTURE_TYPE_MUTABLE_KERNEL_ARGUMENT_EXP_DESC, chainHead,
            commandId, 0u, sizeof(int), &newKernelArgValue};
        chainHead = &kernelArgDesc;
    }
    if (mutateGroupCount) {
        groupCountDesc = ze_mutable_group_count_exp_desc_t{
            ZE_STRUCTURE_TYPE_MUTABLE_GROUP_COUNT_EXP_DESC, chainHead,
            commandId, &newGroupCount};
        chainHead = &groupCountDesc;
    }
    if (mutateGroupSize) {
        groupSizeDesc = ze_mutable_group_size_exp_desc_t{
            ZE_STRUCTURE_TYPE_MUTABLE_GROUP_SIZE_EXP_DESC, chainHead,
            commandId, 1u, 1u, 1u};
        chainHead = &groupSizeDesc;
    }
    if (mutateGlobalOffset) {
        globalOffsetDesc = ze_mutable_global_offset_exp_desc_t{
            ZE_STRUCTURE_TYPE_MUTABLE_GLOBAL_OFFSET_EXP_DESC, chainHead,
            commandId, 0u, 0u, 0u};
        chainHead = &globalOffsetDesc;
    }

    ze_mutable_commands_exp_desc_t mutableCommandsDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMANDS_EXP_DESC, chainHead, 0u};

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandsExp(mutableCmdList, &mutableCommandsDesc));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandsExp(mutableCmdList, &mutableCommandsDesc));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
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
