/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/timer.h"

#include <math.h>
#include <random>

struct SinKernelGraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numKernels;
    BooleanArgument withGraphs;
    BooleanArgument withCopyOffload;
    BooleanArgument immediateAppendCmdList;

    SinKernelGraphArguments()
        : numKernels(*this, "numKernels", "Number of kernel invocations"),
          withGraphs(*this, "withGraphs", "Runs with or without graphs"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)"),
          immediateAppendCmdList(*this, "immediateAppendCmdList", "Use zeCommandListImmediateAppendCommandListsExp to submit graph (only valid for L0)") {}
};

struct SinKernelGraph : TestCase<SinKernelGraphArguments> {
    using TestCase<SinKernelGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SinKernelGraph";
    }

    std::string getHelp() const override {
        return "Benchmark running memory copy and kernel runs, with graphs and without graphs";
    }
};
