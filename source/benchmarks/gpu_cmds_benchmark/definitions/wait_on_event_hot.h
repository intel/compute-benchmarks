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

struct WaitOnEventHotArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;

    WaitOnEventHotArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()) {}
};

struct WaitOnEventHot : TestCase<WaitOnEventHotArguments> {
    using TestCase<WaitOnEventHotArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WaitOnEventHot";
    }

    std::string getHelp() const override {
        return "measures time required to service a signalled semaphore, that was previously used";
    }
};
