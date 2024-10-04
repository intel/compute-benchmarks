/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitGraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;

    SubmitGraphArguments()
        : numKernels(*this, "NumKernels", "Number of kernels to submit to the queue") {}
};

struct SubmitGraph : TestCase<SubmitGraphArguments> {
    using TestCase<SubmitGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitGraph";
    }

    std::string getHelp() const override {
        return "measures time spent in submitting a graph to a SYCL (or SYCL-like) queue";
    }
};
