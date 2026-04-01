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

struct AggregatedEventSignalingArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;

    AggregatedEventSignalingArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()) {}
};

struct AggregatedEventSignaling : TestCase<AggregatedEventSignalingArguments> {
    using TestCase<AggregatedEventSignalingArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AggregatedEventSignaling";
    }

    std::string getHelp() const override {
        return "measures time required to signal an aggregated event, where each operation generates an atomic write";
    }
};
