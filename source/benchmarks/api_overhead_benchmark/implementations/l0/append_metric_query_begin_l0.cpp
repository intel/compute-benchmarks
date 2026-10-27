/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_metric_query_begin.h"
#include "utility/l0/metric_query_utility_l0.h"

static TestResult run(const AppendMetricQueryArguments &arguments, Statistics &statistics) {
    const bool useSignalEvent = false;
    return measureMetricQueryAppend(arguments, statistics, MetricQueryAppend::Begin, useSignalEvent);
}

static RegisterTestCaseImplementation<AppendMetricQueryBegin> registerTestCase(run, Api::L0);
