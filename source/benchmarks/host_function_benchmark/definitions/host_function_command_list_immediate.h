/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct HostFunctionCommandListImmediateArguments : TestCaseArgumentContainer {
    BooleanArgument measureCompletionTime;
    BooleanArgument useEmptyHostFunction;
    BooleanArgument useKernels;
    PositiveIntegerArgument kernelExecutionTime;
    PositiveIntegerArgument amountOfCalls;

    HostFunctionCommandListImmediateArguments() : measureCompletionTime(*this, "MeasureCompletion", "Add completion time in the measurement."),
                                                  useEmptyHostFunction(*this, "UseEmptyHostFunction", "When enabled, the test will use an empty host function, otherwise it will use a function with a 1ms busy spin."),
                                                  useKernels(*this, "UseKernels", "Use kernels and a host function in between during the test."),
                                                  kernelExecutionTime(*this, "KernelExecTime", "Execution time of a single kernel (in us)."),
                                                  amountOfCalls(*this, "CallsCount", "The number of calls that are being measured.") {
    }
};

struct HostFunctionCommandListImmediate : TestCase<HostFunctionCommandListImmediateArguments> {
    using TestCase<HostFunctionCommandListImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "HostFunctionCmdListImmediate";
    }

    std::string getHelp() const override {
        return "Measures the overhead of the host function.";
    }
};
