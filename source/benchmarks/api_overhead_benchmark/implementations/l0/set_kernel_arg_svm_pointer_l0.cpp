/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/set_kernel_arg_svm_pointer.h"

#include <gtest/gtest.h>

typedef struct _st_container st_container;

struct _st_container {
    int32_t *value;
    st_container *next;
};

static TestResult run(const SetKernelArgSvmPointerArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernels
    const auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_indirect_access_kernel.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    std::vector<ze_kernel_handle_t> kernels(arguments.allocationsCount);
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "indirectAccess";
    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ze_kernel_handle_t kernel;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
        kernels[i] = kernel;
    }

    // Configure kernel
    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1u, 1u, 1u));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetIndirectAccess(kernels[i], ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST | ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE | ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED));
    }

    // Create allocations
    std::vector<void *> allocations(arguments.allocationsCount);
    for (auto i = 0u; i < arguments.allocationsCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Shared, levelzero, arguments.allocationSize, &allocations[i]));
    }

    // Reallocate if argument is set
    if (arguments.reallocate) {
        for (auto i = 0u; i < arguments.allocationsCount; ++i) {
            ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Shared, levelzero, allocations[i]));
            ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Shared, levelzero, arguments.allocationSize, &allocations[i]));
        }
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto j = 0u; j < arguments.allocationsCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[j], 0, sizeof(void *), &allocations[j]));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
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
