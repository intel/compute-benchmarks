/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/error.h"
#include "framework/ol/ol.h"
#include "framework/ol/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_batch_memory_allocation.h"

#include <gtest/gtest.h>

static TestResult run(const UsmBatchMemoryAllocationArguments &arguments,
                      Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds,
                                   MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(),
                                   typeSelector.getType());
        return TestResult::Nooped;
    }

    OlState ol;
    Timer timer;

    std::vector<void *> ptrs(arguments.allocationCount);

    for (auto j = 0u; j < arguments.iterations; j++) {
        if (arguments.measureMode == AllocationMeasureMode::Allocate ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureStart();
        }

        for (auto i = 0u; i < arguments.allocationCount; i++) {
            ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::allocate(
                arguments.usmMemoryPlacement, ol.context, ol.device,
                arguments.size, &ptrs[i]));
        }

        if (arguments.measureMode == AllocationMeasureMode::Allocate) {
            timer.measureEnd();
        } else if (arguments.measureMode == AllocationMeasureMode::Free) {
            timer.measureStart();
        }

        for (auto i = 0u; i < arguments.allocationCount; i++) {
            ASSERT_OL_RESULT_SUCCESS(olMemFree(ol.context, ptrs[i]));
        }

        if (arguments.measureMode == AllocationMeasureMode::Free ||
            arguments.measureMode == AllocationMeasureMode::Both) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(),
                             typeSelector.getType());
    }
    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<UsmBatchMemoryAllocation>
    registerTestCase(run, Api::OL);
