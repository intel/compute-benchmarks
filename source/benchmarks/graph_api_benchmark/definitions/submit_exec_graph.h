/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitExecGraphArguments : TestCaseArgumentContainer {
    BooleanArgument measureSubmit;
    PositiveIntegerArgument numKernels;
    BooleanArgument ioq;

    SubmitExecGraphArguments()
        : measureSubmit(*this, "measureSubmit", "If true, the benchmark measures graph submission time, otherwise - graph execution time."),
          numKernels(*this, "numKernels", "Number of kernels to exec in a graph"),
          ioq(*this, "ioq", "Use in-order queue") {}
};

struct SubmitExecGraph : TestCase<SubmitExecGraphArguments> {
    using TestCase<SubmitExecGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitExecGraph";
    }

    std::string getHelp() const override {
        return "The benchmark measures submission time or execution time of a SYCL graph";
    }
};
