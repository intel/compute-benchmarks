/*
 * Copyright (C) 2022-2023 Intel Corporation
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

    // Warmup
    ze_driver_properties_t driverPropertiesWarmUp{};
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGetProperties(drivers[0], &driverPropertiesWarmUp));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_driver_properties_t driverProperties{};

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeDriverGetProperties(drivers[0], &driverProperties));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<DriverGetProperties> registerTestCase(run, Api::L0);
