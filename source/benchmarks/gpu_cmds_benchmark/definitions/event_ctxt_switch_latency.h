/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct EventCtxtSwitchLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;
    EngineArgument firstEngine;
    EngineArgument secondEngine;

    EventCtxtSwitchLatencyArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()),
          firstEngine(*this, "firstEngine", "first engine to measure context switch latency"),
          secondEngine(*this, "secondEngine", "second engine to measure context switch latency") {}
};

struct EventCtxtSwitchLatency : TestCase<EventCtxtSwitchLatencyArguments> {
    using TestCase<EventCtxtSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventCtxtSwitchLatency";
    }

    std::string getHelp() const override {
        return "measures context switching latency time required to switch between various engine types";
    }
};
