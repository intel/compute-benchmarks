/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CreateCommandListArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;
    BooleanArgument copyOnly;

    CreateCommandListArguments()
        : cmdListCount(*this, "CmdListCount", "Number of cmdlists to create"),
          copyOnly(*this, "CopyOnly", "Create copy only cmdlist") {}
};

struct CreateCommandList : TestCase<CreateCommandListArguments> {
    using TestCase<CreateCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CreateCommandList";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListCreate on CPU.";
    }
};
