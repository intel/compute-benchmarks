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

#include "definitions/command_list_update_mutable_command_kernels.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const CommandListUpdateMutableCommandKernelsArguments &arguments, Statistics &statistics) {
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

    if (!(levelzero.getDeviceMclProperties().mutableCommandFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION)) {
        return TestResult::DeviceNotCapable;
    }

    const uint32_t numKernels = static_cast<uint32_t>(arguments.numKernels);
    if (numKernels == 0u) {
        return TestResult::InvalidArgs;
    }

    // Create mutable command list
    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    // Create module + N kernels that can be swapped between.
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

    std::vector<ze_kernel_handle_t> kernels(numKernels, nullptr);
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    for (uint32_t i = 0; i < numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1u, 1u, 1u));
        const int initialKernelOpsCount = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(int), &initialKernelOpsCount));
    }

    // Register the mutable command with the full kernel candidate list
    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr,
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
            ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
            ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
            ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION};
    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp(
        mutableCmdList, &commandIdDesc, kernels.size(), kernels.data(), &commandId));

    ze_group_count_t initialGroupCount = {1u, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernels[0],
                                                             &initialGroupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    // Warmup
    {
        uint64_t warmupCommandId = commandId;
        ze_kernel_handle_t warmupKernel = kernels[numKernels > 1u ? 1u : 0u];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandKernelsExp(
            mutableCmdList, 1u, &warmupCommandId, &warmupKernel));
    }

    // Benchmark - cycle through kernel candidates.
    for (auto j = 0u; j < arguments.iterations; j++) {
        uint64_t iterCommandId = commandId;
        ze_kernel_handle_t newKernel = kernels[j % numKernels];

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandKernelsExp(
            mutableCmdList, 1u, &iterCommandId, &newKernel));
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

static RegisterTestCaseImplementation<CommandListUpdateMutableCommandKernels> registerTestCase(run, Api::L0);
