/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecGraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;

    ExecGraphArguments()
        : numKernels(*this, "NumKernels", "Number of kernels to exec in a graph") {}
};

struct ExecGraph : TestCase<ExecGraphArguments> {
    using TestCase<ExecGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecGraph";
    }

    std::string getHelp() const override {
        return "Measures time spent in execution of a SYCL graph";
    }
};
