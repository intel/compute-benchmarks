/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListHostSynchronizeArguments : TestCaseArgumentContainer {
    BooleanArgument useBarrierBeforeSync;

    CommandListHostSynchronizeArguments()
        : useBarrierBeforeSync(*this, "UseBarrierBeforeSync", "Append an event-signalling-barrier before synchronization") {}
};

struct CommandListHostSynchronize : TestCase<CommandListHostSynchronizeArguments> {
    using TestCase<CommandListHostSynchronizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListHostSynchronize";
    }

    std::string getHelp() const override {
        return "measures CPU time spent in zeCommandListHostSynchronize. Optionally, adds an event-signalling barrier"
               "and waits for the event, before calling zeCommandListHostSynchronize";
    }
};
