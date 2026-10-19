/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct GraphReplayLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument replayCount;
    NonNegativeIntegerArgument forkCount;

    GraphReplayLatencyArguments()
        : replayCount(*this, "replayCount", "Number of times the graph is appended in a single iteration, must be at least 2"),
          forkCount(*this, "forkCount", "Number of forks in the graph, each fork launches a single kernel") {}

    bool validateArgumentsExtra() const override {
        return replayCount >= 2;
    }
};

struct GraphReplayLatency : TestCase<GraphReplayLatencyArguments> {
    using TestCase<GraphReplayLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "GraphReplayLatency";
    }

    std::string getHelp() const override {
        return "records a graph and appends it replayCount times to an in-order immediate command list, each time with a profiling event. "
               "Measures time from the end of one graph execution till the start of the next one.";
    }
};
