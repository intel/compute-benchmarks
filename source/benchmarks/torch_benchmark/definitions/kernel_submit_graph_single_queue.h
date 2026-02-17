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

struct KernelSubmitGraphSingleQueueArguments : TestCaseArgumentContainer {
    KernelNameArgument kernelName;
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    Uint32Argument kernelGroupsCount;
    PositiveIntegerArgument kernelBatchSize;
    BooleanArgument useProfiling;
    BooleanArgument useEvents;

    KernelSubmitGraphSingleQueueArguments() : kernelName(*this, "kernelName", "Name of the kernel."),
                                              kernelWGCount(*this, "kernelWGCount", "Number of workgroups."),
                                              kernelWGSize(*this, "kernelWGSize", "Size of each workgroup."),
                                              kernelGroupsCount(*this, "kernelGroupsCount", "Number of nodes (kernel groups) recorded in a graph."),
                                              kernelBatchSize(*this, "kernelBatchSize", "Size of a batch of kernels after which synchronization occurs."),
                                              useProfiling(*this, "useProfiling", "Create the queue with the enable_profiling property (SYCL only)."),
                                              useEvents(*this, "UseEvents", "Use events when enqueuing kernels.") {}
};

struct KernelSubmitGraphSingleQueue : TestCase<KernelSubmitGraphSingleQueueArguments> {
    using TestCase::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitGraphSingleQueue";
    }

    std::string getHelp() const override {
        return "Measures the performance of batches of submission + synchronization of kernels using a captured command graph on a single in-order queue.";
    }
};
