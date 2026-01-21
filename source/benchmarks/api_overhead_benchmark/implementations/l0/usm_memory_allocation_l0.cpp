/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_memory_allocation.h"

#include <gtest/gtest.h>

static TestResult run(const UsmMemoryAllocationArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    void *ptr{};

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        if (arguments.measureMode == AllocationMeasureMode::Allocate ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureStart();
        }
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.usmMemoryPlacement, levelzero, arguments.size, &ptr));

        if (arguments.measureMode == AllocationMeasureMode::Allocate) {
            timer.measureEnd();
        } else if (arguments.measureMode == AllocationMeasureMode::Free) {
            timer.measureStart();
        }

        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, ptr));

        if (arguments.measureMode == AllocationMeasureMode::Free ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmMemoryAllocation> registerTestCase(run, Api::L0);
