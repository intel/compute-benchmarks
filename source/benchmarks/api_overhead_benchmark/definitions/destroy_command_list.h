/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct DestroyCommandListArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;

    DestroyCommandListArguments()
        : cmdListCount(*this, "CmdListCount", "Number of cmdlists to destroy") {}
};

struct DestroyCommandList : TestCase<DestroyCommandListArguments> {
    using TestCase<DestroyCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DestroyCommandList";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListDestroy on CPU.";
    }
};
