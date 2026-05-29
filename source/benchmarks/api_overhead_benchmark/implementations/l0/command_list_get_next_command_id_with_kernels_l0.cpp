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

#include "definitions/command_list_get_next_command_id_with_kernels.h"

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
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION |
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
    case MutableCommandFlag::KernelInstruction:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
    case MutableCommandFlag::GraphArguments:
        return ZE_MUTABLE_COMMAND_EXP_FLAG_GRAPH_ARGUMENTS;
    default:
        return 0;
    }
}

} // namespace

static TestResult run(const CommandListGetNextCommandIdWithKernelsArguments &arguments, Statistics &statistics) {
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

    if (!levelzero.isMclExtensionAvailable(1, 1)) {
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

    // Create kernel
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

    std::vector<ze_kernel_handle_t> kernels(arguments.numKernels, nullptr);
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";

    for (size_t i = 0; i < arguments.numKernels; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(int), &i));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1u, 1u, 1u));
    }

    // Create mutable command identifier descriptor
    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};
    uint64_t mutableKernelCommandId = 0;

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp(
        mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(), &mutableKernelCommandId));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp(
            mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(), &mutableKernelCommandId));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (auto k : kernels) {
        if (k) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(k));
        }
    }

    if (module) {
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    }

    if (mutableCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mutableCmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListGetNextCommandIdWithKernels> registerTestCase(run, Api::L0);