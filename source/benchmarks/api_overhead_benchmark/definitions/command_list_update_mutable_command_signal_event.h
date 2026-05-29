/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListUpdateMutableCommandSignalEventArguments : TestCaseArgumentContainer {};

struct CommandListUpdateMutableCommandSignalEvent : TestCase<CommandListUpdateMutableCommandSignalEventArguments> {
    using TestCase<CommandListUpdateMutableCommandSignalEventArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListUpdateMutableCommandSignalEvent";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListUpdateMutableCommandSignalEventExp call on CPU. "
               "A single mutable kernel command is appended with an initial signal event. "
               "Each iteration swaps the command's signal event to an alternate handle. ";
    }
};
