/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListImmediateGetIndexArguments : TestCaseArgumentContainer {
    CommandListImmediateGetIndexArguments() {}
};

struct CommandListImmediateGetIndex : TestCase<CommandListImmediateGetIndexArguments> {
    using TestCase<CommandListImmediateGetIndexArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListImmediateGetIndex";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListImmediateGetIndex call on CPU.";
    }
};
