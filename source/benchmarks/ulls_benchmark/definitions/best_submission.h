/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct BestSubmissionArguments : TestCaseArgumentContainer {};

struct BestSubmission : TestCase<BestSubmissionArguments> {
    using TestCase<BestSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BestSubmission";
    }

    std::string getHelp() const override {
        return "enqueues a system memory write via PIPE_CONTROL and measures when update becomes "
               "visible on the CPU.";
    }
};
