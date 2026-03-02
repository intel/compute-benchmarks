/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitMultiQueueArguments : TestCaseArgumentContainer {

    PositiveIntegerArgument kernelWGCount;
    PositiveIntegerArgument kernelWGSize;
    PositiveIntegerArgument kernelsPerQueue;
    BooleanArgument useProfiling;
    BooleanArgument measureCompletionTime;
    BooleanArgument useEvents;

    KernelSubmitMultiQueueArguments() : kernelWGCount(*this, "KernelWGCount", "Number of workgroups."),
                                        kernelWGSize(*this, "KernelWGSize", "Size of each workgroup."),
                                        kernelsPerQueue(*this, "KernelsPerQueue", "Number of kernels per queue."),
                                        useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property"),
                                        measureCompletionTime(*this, "MeasureCompletionTime", "Measures total time taken to complete all submissions and waits at the end of iteration. By default only kernel submission time is measured."),
                                        useEvents(*this, "UseEvents", "Use events when enqueuing kernels.") {}
};

struct KernelSubmitMultiQueue : TestCase<KernelSubmitMultiQueueArguments> {

    std::string getTestCaseName() const override {
        return "KernelSubmitMultiQueue";
    }

    std::string getHelp() const override {
        return "Measures submit kernel from 2 queues.";
    }
};
