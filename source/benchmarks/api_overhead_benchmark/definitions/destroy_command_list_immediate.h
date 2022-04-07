/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct DestroyCommandListImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;

    DestroyCommandListImmediateArguments()
        : cmdListCount(*this, "CmdListCount", "Number of immediate cmdlists to create") {}
};

struct DestroyCommandListImmediate : TestCase<DestroyCommandListImmediateArguments> {
    using TestCase<DestroyCommandListImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DestroyCommandListImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListDestroy on CPU, for immediate cmdlist.";
    }
};
