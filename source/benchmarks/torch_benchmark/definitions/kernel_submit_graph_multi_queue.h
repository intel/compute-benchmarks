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
    Uint32Argument workgroupCount;
    Uint32Argument workgroupSize;
    Uint32Argument kernelsPerQueue;
    BooleanArgument useProfiling;

    KernelSubmitGraphMultiQueueArguments() : workgroupCount(*this, "workgroupCount", "Number of workgroups."),
                                             workgroupSize(*this, "workgroupSize", "Size of each workgroup."),
                                             kernelsPerQueue(*this, "kernelsPerQueue", "Number of nodes (kernel groups) recorded in a graph"),
                                             useProfiling(*this, "Profiling", "Create queues with profiling enabled.") {}
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
