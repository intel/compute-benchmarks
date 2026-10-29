/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct LifecycleCommandListArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;
    BooleanArgument copyOnly;

    LifecycleCommandListArguments()
        : cmdListCount(*this, "CmdListCount", "Number of cmdlists to create"),
          copyOnly(*this, "CopyOnly", "Create copy only cmdlist") {}
};

struct LifecycleCommandList : TestCase<LifecycleCommandListArguments> {
    using TestCase<LifecycleCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "LifecycleCommandList";
    }

    std::string getHelp() const override {
        return "measures time spent on CPU to create, close, execute, synchronize and destroy CmdListCount command lists, one at a time.";
    }
};
