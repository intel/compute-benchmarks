/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct HostFunctionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument amountOfCalls;
    BooleanArgument measureCompletionTime;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument useIoq;
    BooleanArgument useHostFunctionColdRun;

    HostFunctionArguments() : amountOfCalls(*this, "CallsCount", "amount of calls that is being meassured"),
                              measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submission (default is to measure only Immediate call)"),
                              kernelExecutionTime(*this, "KernelExecTime", "How long a single kernel executes, in us"),
                              useIoq(*this, "ioq", "Use In order queue"),
                              useHostFunctionColdRun(*this, "HostFunctionColdRun", "When enabled don't warmup host function workers") {
    }
};

struct HostFunction : TestCase<HostFunctionArguments> {
    using TestCase<HostFunctionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "HostFunction";
    }

    std::string getHelp() const override {
        return "measures overhead of host function.";
    }
};
