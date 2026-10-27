/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendMetricStreamerMarkerArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;

    AppendMetricStreamerMarkerArguments()
        : appendCount(*this, "appendCount", "Number of appends to run") {}
};

struct AppendMetricStreamerMarker : TestCase<AppendMetricStreamerMarkerArguments> {
    using TestCase<AppendMetricStreamerMarkerArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMetricStreamerMarker";
    }

    std::string getHelp() const override {
        return "measures time spent in zetCommandListAppendMetricStreamerMarker on CPU. "
               "The metric streamer is opened on the first time based metric group of the device that accepts a streamer marker. "
               "Requires ZET_ENABLE_METRICS=1 and access to the device performance counters.";
    }
};
