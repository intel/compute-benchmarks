/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestWalkerNthSubmissionImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;

    BestWalkerNthSubmissionImmediateArguments()
        : kernelCount(*this, "KernelCount", "Kernel count") {}
};

struct BestWalkerNthSubmissionImmediate : TestCase<BestWalkerNthSubmissionImmediateArguments> {
    using TestCase<BestWalkerNthSubmissionImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerNthSubmissionImmediate";
    }

    std::string getHelp() const override {
        return "enqueues n kernels, which updates system memory location and then busy-loops on CPU "
               "until the update of nth kernel becomes visible. Kernel is enqueued using low-latency immediate "
               "command list, so the test is LevelZero-specific.";
    }
};
