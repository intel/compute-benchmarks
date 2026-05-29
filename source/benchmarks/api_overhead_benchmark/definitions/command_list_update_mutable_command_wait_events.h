/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListUpdateMutableCommandWaitEventsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numWaitEvents;

    CommandListUpdateMutableCommandWaitEventsArguments()
        : numWaitEvents(*this, "numWaitEvents",
                        "Number of wait events the mutable kernel command was appended with and that are swapped each iteration.") {}
};

struct CommandListUpdateMutableCommandWaitEvents : TestCase<CommandListUpdateMutableCommandWaitEventsArguments> {
    using TestCase<CommandListUpdateMutableCommandWaitEventsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListUpdateMutableCommandWaitEvents";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListUpdateMutableCommandWaitEventsExp call on CPU. "
               "A single mutable kernel command is appended with N wait events. "
               "Each iteration replaces the wait event list with an alternate set of N events.";
    }
};
