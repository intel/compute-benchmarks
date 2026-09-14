/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EventQueryKernelTimestampArguments : TestCaseArgumentContainer {
    BooleanArgument useImmediate;
    PositiveIntegerArgument queryCount;

    EventQueryKernelTimestampArguments() : useImmediate(*this, "useImmediate", "Signal the event from an immediate command list instead of a regular one"),
                                           queryCount(*this, "queryCount", "Number of queries performed after the first one") {}
};

struct EventQueryKernelTimestamp : TestCase<EventQueryKernelTimestampArguments> {
    using TestCase<EventQueryKernelTimestampArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventQueryKernelTimestamp";
    }

    std::string getHelp() const override {
        return "Measures time spent in zeEventQueryKernelTimestamp for a completed counter based event with device timestamps. "
               "First is the cost of the first query, Repeated is the per call cost of the queries that follow it.";
    }
};
