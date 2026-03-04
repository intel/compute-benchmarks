/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/kernel_name_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitGraphMultiQueueArguments : TestCaseArgumentContainer {
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    Uint32Argument kernelsPerQueue;
    BooleanArgument useProfiling;
    BooleanArgument useEvents;

    KernelSubmitGraphMultiQueueArguments() : kernelWGCount(*this, "KernelWGCount", "Number of workgroups."),
                                             kernelWGSize(*this, "KernelWGSize", "Size of each workgroup."),
                                             kernelsPerQueue(*this, "KernelsPerQueue", "Number of kernels recorded in a graph"),
                                             useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property (SYCL only)."),
                                             useEvents(*this, "UseEvents", "Use events when enqueuing kernels.") {}
};

struct KernelSubmitGraphMultiQueue : TestCase<KernelSubmitGraphMultiQueueArguments> {
    using TestCase<KernelSubmitGraphMultiQueueArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitGraphMultiQueue";
    }

    std::string getHelp() const override {
        return "Measures the performance of submission + synchronization of kernels using a captured command graph on multiple queues.";
    }
};
