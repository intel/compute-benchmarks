/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct RoundTripPingPongArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument placement;

    RoundTripPingPongArguments()
        : placement(*this, "placement", "Placement of the integer incremented by the kernel") {}
};

struct RoundTripPingPong : TestCase<RoundTripPingPongArguments> {
    using TestCase<RoundTripPingPongArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RoundTripPingPong";
    }

    std::string getHelp() const override {
        return "measures a round trip in which a single work item kernel increments an integer and the CPU "
               "decrements it after the queue is synchronized. For Device placement the command list copies the "
               "integer from and back to a host allocation. For Shared placement the command list advises system memory "
               "as the preferred location of the integer, and the CPU writes the integer before and reads it after "
               "the execution.";
    }
};
