/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitKernelArguments : TestCaseArgumentContainer {
    BooleanArgument useProfiling;
    BooleanArgument inOrderQueue;
    BooleanArgument useEvents;
    PositiveIntegerArgument numKernels;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument measureCompletionTime;

    SubmitKernelArguments()
        : useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property"),
          inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
          useEvents(*this, "UseEvents", "Use events when enqueuing kernels. When false, SYCL will use eventless enqueue functions."),
          numKernels(*this, "NumKernels", "Number of kernels to submit to the queue"),
          kernelExecutionTime(*this, "KernelExecTime", "Approximately how long a single kernel executes, in us"),
          measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submission (default is to measure only submit calls)") {}
};

struct SubmitKernel : TestCase<SubmitKernelArguments> {
    using TestCase<SubmitKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitKernel";
    }

    std::string getHelp() const override {
        return "measures time spent in submitting a kernel to a SYCL (or SYCL-like) queue on CPU.";
    }
};
