/*
 * Copyright (C) 2022 Intel Corporation
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
    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties);
    Timer timer;

    uint32_t driverCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, nullptr));
    EXPECT_NE(0, driverCount);

    std::vector<ze_driver_handle_t> drivers(driverCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGet(&driverCount, drivers.data()));

    // Warmup
    ze_api_version_t driverApiVersionWarmUp;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGetApiVersion(drivers[0], &driverApiVersionWarmUp));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_api_version_t driverApiVersion;

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeDriverGetApiVersion(drivers[0], &driverApiVersion));
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DriverGetApiVersion> registerTestCase(run, Api::L0);
