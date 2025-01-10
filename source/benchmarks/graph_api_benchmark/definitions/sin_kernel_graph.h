/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SinKernelGraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;
    BooleanArgument withGraphs;

    SinKernelGraphArguments()
        : numKernels(*this, "numKernels", "Number of kernel invocations"),
          withGraphs(*this, "withGraphs", "Runs with or without graphs") {}
};

struct SinKernelGraph : TestCase<SinKernelGraphArguments> {
    using TestCase<SinKernelGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SinKernelGraph";
    }

    std::string getHelp() const override {
        return "Benchmark calling sycl::sin kernel & doing mem alloc/dealloc, with graphs and without graphs";
    }
};
