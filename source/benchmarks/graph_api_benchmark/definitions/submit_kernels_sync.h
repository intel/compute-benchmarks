/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitKernelsSyncArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;
    BooleanArgument useEventsForSync;

    SubmitKernelsSyncArguments()
        : numKernels(*this, "numKernels", "Number of kernels to submit to the queue"),
          useEventsForSync(*this, "useEventsForHostSync", "Use events fir hist sync") {}
};

struct SubmitKernelsSync : TestCase<SubmitKernelsSyncArguments> {
    using TestCase<SubmitKernelsSyncArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitKernelsSync";
    }

    std::string getHelp() const override {
        return "Measures time spent in submitting a L0 kernels to the queue";
    }
};
