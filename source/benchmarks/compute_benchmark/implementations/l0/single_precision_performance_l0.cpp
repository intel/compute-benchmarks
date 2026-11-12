/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/single_precision_performance.h"
#include "utility/l0/mad_performance_l0.h"

static TestResult run(const SinglePrecisionPerformanceArguments &arguments, Statistics &statistics) {
    return runMadPerformanceL0(arguments, statistics, MadPerformance::DataType::Float, 1u, arguments.useEvents);
}

static RegisterTestCaseImplementation<SinglePrecisionPerformance> registerTestCase(run, Api::L0);
