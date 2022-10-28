/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/driver_get_properties.h"

#include <gtest/gtest.h>

static TestResult run(const DriverGetPropertiesArguments &arguments, Statistics &statistics) {
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
    ze_driver_properties_t driverProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGetProperties(drivers[0], &driverProperties));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_driver_properties_t driverProperties{};

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeDriverGetProperties(drivers[0], &driverProperties));
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DriverGetProperties> registerTestCase(run, Api::L0);
