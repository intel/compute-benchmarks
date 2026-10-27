/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendMetricMemoryBarrierArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;

    AppendMetricMemoryBarrierArguments()
        : appendCount(*this, "appendCount", "Number of appends to run") {}
};

struct AppendMetricMemoryBarrier : TestCase<AppendMetricMemoryBarrierArguments> {
    using TestCase<AppendMetricMemoryBarrierArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMetricMemoryBarrier";
    }

    std::string getHelp() const override {
        return "measures time spent in zetCommandListAppendMetricMemoryBarrier on CPU. "
               "The barriers are appended between the begin and the end of a metric query from the first event based metric group of the device. "
               "Requires ZET_ENABLE_METRICS=1 and access to the device performance counters.";
    }
};
