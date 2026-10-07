/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/set_kernel_group_size.h"

#include <gtest/gtest.h>

static TestResult run(const SetKernelGroupSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_write_sum_local.cl", "write_sum_local", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    uint32_t groupSizeX{};
    uint32_t groupSizeY{};
    uint32_t groupSizeZ{};

    if (arguments.asymmetricLocalWorkSize) {
        groupSizeX = 5u;
        groupSizeY = 5u;
        groupSizeZ = 5u;
    } else {
        groupSizeX = 4u;
        groupSizeY = 4u;
        groupSizeZ = 4u;
    }

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<SetKernelGroupSize> registerTestCase(run, Api::L0);
