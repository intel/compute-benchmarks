/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_empty_kernel.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "empty";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    // Create fence if neccessary
    ze_fence_handle_t fence{};
    if (arguments.useFence) {
        const ze_fence_desc_t fenceDesc{ZE_STRUCTURE_TYPE_FENCE_DESC};
        ASSERT_ZE_RESULT_SUCCESS(zeFenceCreate(levelzero.commandQueue, &fenceDesc, &fence));
    }

    // Create command list
    const ze_group_count_t dispatchTraits{1u, 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceReset(fence));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, fence));
        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceHostSynchronize(fence, std::numeric_limits<uint64_t>::max()));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        }
        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    if (arguments.useFence) {
        ASSERT_ZE_RESULT_SUCCESS(zeFenceDestroy(fence));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandList> registerTestCase(run, Api::L0);
