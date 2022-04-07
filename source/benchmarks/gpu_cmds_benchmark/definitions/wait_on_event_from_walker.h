/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct WaitOnEventFromWalkerArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;

    WaitOnEventFromWalkerArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()) {}
};

struct WaitOnEventFromWalker : TestCase<WaitOnEventFromWalkerArguments> {
    using TestCase<WaitOnEventFromWalkerArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WaitOnEventFromWalker";
    }

    std::string getHelp() const override {
        return "measures time required to service a signalled semaphore coming from Walker command";
    }
};
