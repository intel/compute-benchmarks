/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListImmediateMultiKernelArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument amountOfCalls;
    PositiveIntegerArgument kernelExecutionTime;

    ExecuteCommandListImmediateMultiKernelArguments()
        : amountOfCalls(*this, "CallsCount", "amount of calls that is being measured"),
          kernelExecutionTime(*this, "KernelExecutionTime", "How long a single kernel executes, in us") {}
};

struct ExecuteCommandListImmediateMultiKernel : TestCase<ExecuteCommandListImmediateMultiKernelArguments> {
    using TestCase<ExecuteCommandListImmediateMultiKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListImmediateMultiKernel";
    }

    std::string getHelp() const override {
        return "measures time spent in executing multiple instances of two different kernels with immediate command list on CPU.";
    }
};
