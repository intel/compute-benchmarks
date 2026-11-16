/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/set_kernel_arg_buffer.h"
#include "utility/l0/kernel_set_argument_value_utility_l0.h"

using namespace L0;

static TestResult run(const KernelSetArgumentValueBufferArguments &arguments, Statistics &statistics) {
    if (isNoopRun()) {
        statistics.pushUnitAndType(MeasurementUnit::Microseconds, MeasurementType::Cpu);
        return TestResult::Nooped;
    }
    if (arguments.argumentIndex >= kernelSetArgumentValueArgumentCount) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    constexpr size_t bufferSize = 4096u;

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_multi_arg_kernel.cl", "kernelWith8", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    std::array<void *, 2> buffers{};
    for (auto &buffer : buffers) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferSize, &buffer));
    }

    const TestResult result = measureKernelSetArgumentValue(kernel, arguments.argumentIndex, buffers, arguments.differentValues, arguments.iterations, statistics);

    for (auto &buffer : buffers) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, buffer));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return result;
}

static RegisterTestCaseImplementation<KernelSetArgumentValueBuffer> registerTestCase(run, Api::L0);
