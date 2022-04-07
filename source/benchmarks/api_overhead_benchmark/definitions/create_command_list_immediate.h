/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CreateCommandListImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;

    CreateCommandListImmediateArguments()
        : cmdListCount(*this, "CmdListCount", "Number of cmdlists to create") {}
};

struct CreateCommandListImmediate : TestCase<CreateCommandListImmediateArguments> {
    using TestCase<CreateCommandListImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CreateCommandListImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListCreateImmediate on CPU.";
    }
};
