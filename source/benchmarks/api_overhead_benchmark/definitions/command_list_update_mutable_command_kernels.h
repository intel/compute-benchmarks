/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListUpdateMutableCommandKernelsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;

    CommandListUpdateMutableCommandKernelsArguments()
        : numKernels(*this, "numKernels",
                     "Number of kernels registered with the mutable command that can be swapped in via zeCommandListUpdateMutableCommandKernelsExp.") {}
};

struct CommandListUpdateMutableCommandKernels : TestCase<CommandListUpdateMutableCommandKernelsArguments> {
    using TestCase<CommandListUpdateMutableCommandKernelsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListUpdateMutableCommandKernels";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListUpdateMutableCommandKernelsExp call on CPU. "
               "A single mutable kernel command is appended with one initial kernel selected from a list of N candidates. "
               "Each iteration swaps the active kernel to the next one in the list.";
    }
};
