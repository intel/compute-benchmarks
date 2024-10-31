/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SinKernelGraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument multiplier;
    BooleanArgument run_with_graph;

    SinKernelGraphArguments()
        : multiplier(*this, "multiplier", "Number of kernel invocations"),
          run_with_graph(*this, "run_with_graph", "Runs with or without graphs") {}
};

struct SinKernelGraph : TestCase<SinKernelGraphArguments> {
    using TestCase<SinKernelGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SinKernelGraph";
    }

    std::string getHelp() const override {
        return "Measures time spent in SinKernelGraph";
    }
};
