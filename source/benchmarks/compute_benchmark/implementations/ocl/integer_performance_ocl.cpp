/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/integer_performance.h"
#include "utility/ocl/mad_performance_ocl.h"

static TestResult run(const IntegerPerformanceArguments &arguments, Statistics &statistics) {
    return runMadPerformanceOcl(arguments, statistics, MadPerformance::DataType::Int32, arguments.vectorWidth, arguments.useEvents);
}

static RegisterTestCaseImplementation<IntegerPerformance> registerTestCase(run, Api::OpenCL);
