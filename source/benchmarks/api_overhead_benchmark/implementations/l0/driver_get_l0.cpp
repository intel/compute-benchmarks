/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/driver_get.h"

#include <gtest/gtest.h>

static TestResult run(const DriverGetArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties);
    Timer timer;

    // Benchmark
    uint32_t driverCount = 0;
    for (auto j = 0u; j < arguments.iterations; j++) {
        if (arguments.getDriverCount) {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
            timer.measureEnd();
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
            EXPECT_NE(0u, driverCount);
            std::vector<ze_driver_handle_t> drivers(driverCount);
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, drivers.data()));
            timer.measureEnd();
        }

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        driverCount = 0;
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DriverGet> registerTestCase(run, Api::L0);
