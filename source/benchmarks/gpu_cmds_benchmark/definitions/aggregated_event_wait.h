/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct AggregatedEventWaitArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;

    AggregatedEventWaitArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()) {}
};

struct AggregatedEventWait : TestCase<AggregatedEventWaitArguments> {
    using TestCase<AggregatedEventWaitArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AggregatedEventWait";
    }

    std::string getHelp() const override {
        return "measures time required to service an aggregated event wait using a single wait on a GPU";
    }
};
