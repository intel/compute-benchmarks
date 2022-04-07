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

struct WaitOnEventColdArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;

    WaitOnEventColdArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()) {}
};

struct WaitOnEventCold : TestCase<WaitOnEventColdArguments> {
    using TestCase<WaitOnEventColdArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WaitOnEventCold";
    }

    std::string getHelp() const override {
        return "measures time required to service a signalled semaphore, that has never been waited for.";
    }
};
