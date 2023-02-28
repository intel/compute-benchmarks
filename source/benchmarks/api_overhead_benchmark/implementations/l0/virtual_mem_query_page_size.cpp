/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_query_page_size.h"

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include <gtest/gtest.h>

static TestResult run(const VirtualMemQueryPageSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    (void)arguments;

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Warmup
    size_t pageSize = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       (MemoryConstants::gigaByte * 1) - 1u, &pageSize));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        uint32_t implicitIterationCount = 100;
        timer.measureStart();
        // Use implicit iteration count to have large samples
        for (auto j = 0u; j < implicitIterationCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                               (MemoryConstants::gigaByte * 1) - 1u, &pageSize));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / implicitIterationCount, typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemQueryPageSize> registerTestCase(run, Api::L0);
