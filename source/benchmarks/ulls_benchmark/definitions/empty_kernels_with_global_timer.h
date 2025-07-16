/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct EmptyKernelsWithGlobalTimerArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;

    EmptyKernelsWithGlobalTimerArguments()
        : kernelCount(*this, "count", "Count of kernels") {}
};

struct EmptyKernelsWithGlobalTimer : TestCase<EmptyKernelsWithGlobalTimerArguments> {
    using TestCase<EmptyKernelsWithGlobalTimerArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EmptyKernelsWithGlobalTimer";
    }

    std::string getHelp() const override {
        return "submits multiple kernels with signal events and barriers in single command lists, uses global timestamps to get total time of their execution";
    }
};
