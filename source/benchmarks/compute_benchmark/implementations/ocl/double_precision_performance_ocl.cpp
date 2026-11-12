/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/double_precision_performance.h"
#include "utility/ocl/mad_performance_ocl.h"

static TestResult run(const DoublePrecisionPerformanceArguments &arguments, Statistics &statistics) {
    return runMadPerformanceOcl(arguments, statistics, MadPerformance::DataType::Double, arguments.vectorWidth, arguments.useEvents);
}

static RegisterTestCaseImplementation<DoublePrecisionPerformance> registerTestCase(run, Api::OpenCL);
