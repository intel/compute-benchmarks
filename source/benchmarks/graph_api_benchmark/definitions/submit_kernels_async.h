/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitKernelsAsyncArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;
    BooleanArgument useEventsForSync;

    SubmitKernelsAsyncArguments()
        : numKernels(*this, "numKernels", "Number of kernels to submit to the queue"),
          useEventsForSync(*this, "useEventsForHostSync", "Use events fir hist sync") {}
};

struct SubmitKernelsAsync : TestCase<SubmitKernelsAsyncArguments> {
    using TestCase<SubmitKernelsAsyncArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitKernelsAsync";
    }

    std::string getHelp() const override {
        return "Measures time spent in submitting a L0 kernels to the queue";
    }
};
