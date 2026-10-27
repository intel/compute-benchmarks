/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_metric_query_end.h"
#include "utility/l0/metric_query_utility_l0.h"

static TestResult run(const AppendMetricQueryEndArguments &arguments, Statistics &statistics) {
    return measureMetricQueryAppend(arguments, statistics, MetricQueryAppend::End, arguments.useEvent);
}

static RegisterTestCaseImplementation<AppendMetricQueryEnd> registerTestCase(run, Api::L0);
