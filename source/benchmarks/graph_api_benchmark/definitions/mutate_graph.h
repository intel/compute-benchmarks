/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MutateGraphArguments : TestCaseArgumentContainer {
    BooleanArgument canUpdate;
    PositiveIntegerArgument numKernels;
    PositiveIntegerArgument changeRate;
    BooleanArgument compareCreation;

    MutateGraphArguments()
        : canUpdate(*this, "canUpdate", "If true, the benchmark modification using mutable calls, otherwise it creation from scratch."),
          numKernels(*this, "numKernels", "Number of kernels to exec in a graph"),
          changeRate(*this, "changeRate", "What is a rate in which kernels should be changed (for exmaple 5 means that every 5th kernel will be changed)"),
          compareCreation(*this, "compareCreation", "If true, the benchmark time needed to create command list, otherwise time needed to modify") {}
};

struct MutateGraph : TestCase<MutateGraphArguments> {
    using TestCase<MutateGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MutateGraph";
    }

    std::string getHelp() const override {
        return "The benchmark measures modification time, compared to time needed to recreate graph";
    }
};
