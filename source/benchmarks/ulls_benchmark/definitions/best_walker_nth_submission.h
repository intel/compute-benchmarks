/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestWalkerNthSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;

    BestWalkerNthSubmissionArguments()
        : kernelCount(*this, "KernelCount", "Kernel count") {}
};

struct BestWalkerNthSubmission : TestCase<BestWalkerNthSubmissionArguments> {
    using TestCase<BestWalkerNthSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerNthSubmission";
    }

    std::string getHelp() const override {
        return "enqueues n kernels, which updates system memory location and then busy-loops on CPU "
               "until the update of nth kernel becomes visible.";
    }
};
