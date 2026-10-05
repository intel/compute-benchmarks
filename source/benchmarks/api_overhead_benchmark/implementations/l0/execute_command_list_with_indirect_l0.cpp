/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_with_indirect.h"

#include <gtest/gtest.h>

static TestResult
run(const ExecuteCommandListWithIndirectArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_empty_kernel.cl", "empty", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetIndirectAccess(kernel, ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST | ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE | ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED));
    // create indirect allocations

    std::vector<void *> indirectAllocations;

    for (uint32_t i = 0; i < arguments.IndirectAllocationsAmount; i++) {
        void *ptr = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(arguments.placement, levelzero, 4096u, &ptr));
        indirectAllocations.push_back(ptr);
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

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    }

    for (uint32_t i = 0; i < arguments.IndirectAllocationsAmount; i++) {

        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, indirectAllocations[i]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListWithInidrect> registerTestCase(run, Api::L0);
