/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestWalkerSubmissionImmediateArguments : TestCaseArgumentContainer {};

struct BestWalkerSubmissionImmediate : TestCase<BestWalkerSubmissionImmediateArguments> {
    using TestCase<BestWalkerSubmissionImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerSubmissionImmediate";
    }

    std::string getHelp() const override {
        return "enqueues kernel, which updates system memory location and then busy-loops on CPU "
               "until the update becomes visible. Kernel is enqueued using low-latency immediate "
               "command list, so the test is LevelZero-specific.";
    }
};
