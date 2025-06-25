/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SubmitGraphArguments : TestCaseArgumentContainer {
    BooleanArgument useProfiling;
    BooleanArgument useHostTasks;
    BooleanArgument inOrderQueue;
    BooleanArgument useEvents;
    BooleanArgument useExplicit;
    PositiveIntegerArgument numKernels;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument measureCompletionTime;

    SubmitGraphArguments()
        : useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property"),
          useHostTasks(*this, "UseHostTasks", "Submit SYCL host task after kernel enqueue"),
          inOrderQueue(*this, "InOrderQueue", "Create the queue with the in_order property"),
          useEvents(*this, "UseEvents", "Do not create and track events for synchronization"),
          useExplicit(*this, "UseExplicit", "Using Explicit Graph Creation mode vs. Record and Replay"),
          numKernels(*this, "NumKernels", "Number of kernels to submit to the queue"),
          kernelExecutionTime(*this, "KernelExecutionTime", "Approximately how long a single kernel executes, in us"),
          measureCompletionTime(*this, "MeasureCompletionTime", "Measures time taken to complete the submission (default is to measure only submit calls)") {}
};

struct SubmitGraph : TestCase<SubmitGraphArguments> {
    using TestCase<SubmitGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SubmitGraph";
    }

    std::string getHelp() const override {
        return "measures time spent in submitting a graph to a SYCL (or SYCL-like) queue on CPU.";
    }
};
