/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/driver_get_api_version.h"

#include <gtest/gtest.h>

static TestResult run(const DriverGetApiVersionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties);
    Timer timer;

    uint32_t driverCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
    EXPECT_NE(0u, driverCount);

    std::vector<ze_driver_handle_t> drivers(driverCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, drivers.data()));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_api_version_t driverApiVersion;

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeDriverGetApiVersion(drivers[0], &driverApiVersion));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DriverGetApiVersion> registerTestCase(run, Api::L0);
