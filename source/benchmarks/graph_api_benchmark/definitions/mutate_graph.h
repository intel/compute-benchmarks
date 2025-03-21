/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/graph_operation_type_argument.h"
#include "framework/test_case/test_case.h"

struct MutateGraphArguments : TestCaseArgumentContainer {
    BooleanArgument canUpdate;
    PositiveIntegerArgument changeRate;
    PositiveIntegerArgument numKernels;
    BooleanArgument useInOrder;
    GraphOperationTypeArgument operationType;

    MutateGraphArguments()
        : canUpdate(*this, "canUpdate", "If true, the benchmark graph changes are done using L0 mutable command list features, otherwise they are created every time from scratch."),
          changeRate(*this, "changeRate", "Rate at which kernels should be changed (for example, 5 means that every 5th kernel in the graph will be changed)"),
          numKernels(*this, "numKernels", "Number of kernels to use in a graph"),
          useInOrder(*this, "useInOrder", "If true, lists are created with ZE_COMMAND_LIST_FLAG_IN_ORDER flag, otherwise they are created without it"),
          operationType(*this, "operationType", "Determines operation type that should be measured: creation, modification or execution") {}
};

struct MutateGraph : TestCase<MutateGraphArguments> {
    using TestCase<MutateGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MutateGraph";
    }

    std::string getHelp() const override {
        return "The benchmark quantifies the benefits of using Mutable Command List for graphs compared to recreating the graphs for scratch every time";
    }
};
