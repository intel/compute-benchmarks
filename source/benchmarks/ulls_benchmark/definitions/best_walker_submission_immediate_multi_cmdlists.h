/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct BestWalkerSubmissionImmediateMultiCmdlistsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdlistCount;
    BestWalkerSubmissionImmediateMultiCmdlistsArguments()
        : cmdlistCount(*this, "cmdlistCount", "Count of command lists") {}
};

struct BestWalkerSubmissionImmediateMultiCmdlists : TestCase<BestWalkerSubmissionImmediateMultiCmdlistsArguments> {
    using TestCase<BestWalkerSubmissionImmediateMultiCmdlistsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerSubmissionImmediateMultiCmdlists";
    }

    std::string getHelp() const override {
        return "Append N kernels on N cmdlists, which updates system memory locations and then waits using busy-loop on CPU "
               "until the update becomes visible. Kernels are appended using immediate command lists."
               "Amount of command lists is specified by cmdlistCount.";
    }
};
