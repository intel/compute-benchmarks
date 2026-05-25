/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/event_host_synchronize.h"

static TestResult run(const EventHostSynchronizeArguments &, Statistics &statistics) {
    if (isNoopRun()) {
        MeasurementFields latencySelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
        statistics.pushUnitAndType(latencySelector.getUnit(), latencySelector.getType());
        return TestResult::Nooped;
    }

    return TestResult::NoImplementation;
}

static RegisterTestCaseImplementation<EventHostSynchronize> registerTestCase(run, Api::L0);
