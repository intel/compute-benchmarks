/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include "append_metric_query_arguments.h"

struct AppendMetricQueryBegin : TestCase<AppendMetricQueryArguments> {
    using TestCase<AppendMetricQueryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMetricQueryBegin";
    }

    std::string getHelp() const override {
        return "measures time spent in zetCommandListAppendMetricQueryBegin on CPU. "
               "The metric query pool uses the first event based metric group of the device. "
               "Requires ZET_ENABLE_METRICS=1 and access to the device performance counters.";
    }
};
