/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestWalkerNthCommandListSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument cmdListCount;

    BestWalkerNthCommandListSubmissionArguments()
        : cmdListCount(*this, "CmdListCount", "Command list count") {}
};

struct BestWalkerNthCommandListSubmission : TestCase<BestWalkerNthCommandListSubmissionArguments> {
    using TestCase<BestWalkerNthCommandListSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerNthCommandListSubmission";
    }

    std::string getHelp() const override {
        return "enqueues single kernel on n command lists, which updates system memory location and then busy-loops on CPU "
               "until the update of the kernel of nth command list becomes visible. This is L0 only test.";
    }
};
