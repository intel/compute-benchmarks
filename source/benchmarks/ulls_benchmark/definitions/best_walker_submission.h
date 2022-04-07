/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestWalkerSubmissionArguments : TestCaseArgumentContainer {};

struct BestWalkerSubmission : TestCase<BestWalkerSubmissionArguments> {
    using TestCase<BestWalkerSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestWalkerSubmission";
    }

    std::string getHelp() const override {
        return "enqueues kernel, which updates system memory location and then busy-loops on CPU "
               "until the update becomes visible.";
    }
};
