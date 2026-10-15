/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_update_mutable_command_kernels.h"

#include <gtest/gtest.h>

using namespace L0;
#include <vector>

static TestResult run(const CommandListUpdateMutableCommandKernelsArguments &arguments, Statistics &statistics) {
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

    if (!levelzero.isMclExtensionAvailable(1, 1)) {
        return TestResult::DeviceNotCapable;
    }

    constexpr ze_mutable_command_exp_flags_t mutationFlags =
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_ARGUMENTS |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_COUNT |
        ZE_MUTABLE_COMMAND_EXP_FLAG_GROUP_SIZE |
        ZE_MUTABLE_COMMAND_EXP_FLAG_KERNEL_INSTRUCTION;
    const ze_mutable_command_exp_flags_t deviceFlags = levelzero.getDeviceMclProperties().mutableCommandFlags;
    if ((deviceFlags & mutationFlags) != mutationFlags) {
        return TestResult::DeviceNotCapable;
    }

    const uint32_t numKernels = static_cast<uint32_t>(arguments.numKernels);
    if (numKernels == 0u) {
        return TestResult::InvalidArgs;
    }

    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    std::vector<ze_kernel_handle_t> kernels(numKernels, nullptr);
    ze_module_handle_t module = nullptr;
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernels[0], &module, nullptr);
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    for (uint32_t i = 1; i < numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
    }
    for (uint32_t i = 0; i < numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1u, 1u, 1u));
        const int initialKernelOpsCount = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(int), &initialKernelOpsCount));
    }

    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr, mutationFlags};
    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdWithKernelsExp(
        mutableCmdList, &commandIdDesc, static_cast<uint32_t>(kernels.size()), kernels.data(), &commandId));

    ze_group_count_t initialGroupCount = {1u, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernels[0],
                                                             &initialGroupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    for (auto j = 0u; j < arguments.iterations; j++) {
        uint64_t iterCommandId = commandId;
        ze_kernel_handle_t newKernel = kernels[j % numKernels];

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandKernelsExp(
            mutableCmdList, 1u, &iterCommandId, &newKernel));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto kernel : kernels) {
        if (kernel) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
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
