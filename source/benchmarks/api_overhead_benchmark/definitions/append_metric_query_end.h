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

struct AppendMetricQueryEndArguments : AppendMetricQueryArguments {
    BooleanArgument useEvent;

    AppendMetricQueryEndArguments()
        : useEvent(*this, "event", "Pass a signal event from an event pool to the append call") {}
};

struct AppendMetricQueryEnd : TestCase<AppendMetricQueryEndArguments> {
    using TestCase<AppendMetricQueryEndArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMetricQueryEnd";
    }

    std::string getHelp() const override {
        return "measures time spent in zetCommandListAppendMetricQueryEnd on CPU. "
               "The metric query pool uses the first event based metric group of the device. "
               "Requires ZET_ENABLE_METRICS=1 and access to the device performance counters.";
    }
};
