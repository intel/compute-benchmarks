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

#include "definitions/set_kernel_arg_svm_pointer.h"

#include <gtest/gtest.h>

struct IndirectContainer {
    int32_t *value;
    IndirectContainer *next;
};

static TestResult run(const SetKernelArgSvmPointerArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    ze_module_handle_t module;
    if (auto result = L0::KernelHelper::loadModule(levelzero, "api_overhead_benchmark_indirect_access_kernel.cl", &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    std::vector<ze_kernel_handle_t> kernels(arguments.allocationsCount);
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "indirectAccess";
    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ze_kernel_handle_t kernel;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
        kernels[i] = kernel;
    }

    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1u, 1u, 1u));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetIndirectAccess(kernels[i], ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST | ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE | ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED));
    }

    std::vector<void *> allocations(arguments.allocationsCount);
    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Shared, levelzero, arguments.allocationSize, &allocations[i]));
    }

    if (arguments.reallocate) {
        for (auto i = 0u; i < arguments.allocationsCount; ++i) {
            ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Shared, levelzero, allocations[i]));
            ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Shared, levelzero, arguments.allocationSize, &allocations[i]));
        }
    }

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto j = 0u; j < arguments.allocationsCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[j], 0, sizeof(void *), &allocations[j]));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Shared, levelzero, allocations[i]));
    }

    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SetKernelArgSvmPointer> registerTestCase(run, Api::L0);
