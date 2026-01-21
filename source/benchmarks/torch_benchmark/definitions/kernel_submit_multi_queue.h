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
    BooleanArgument measureCompletion;

    KernelSubmitMultiQueueArguments() : kernelWGCount(*this, "kernelWGCount", "Number of workgroups."),
                                        kernelWGSize(*this, "kernelWGSize", "Size of each workgroup."),
                                        kernelsPerQueue(*this, "kernelsPerQueue", "Number of kernels per queue."),
                                        useProfiling(*this, "useProfiling", "Create the queue with the enable_profiling property"),
                                        measureCompletion(*this, "measureCompletion", "Measures total time taken to complete all submissions and waits at the end of iteration. By default only kernel submission time is measured.") {}
};

struct KernelSubmitMultiQueue : TestCase<KernelSubmitMultiQueueArguments> {

    std::string getTestCaseName() const override {
        return "KernelSubmitMultiQueue";
    }

    std::string getHelp() const override {
        return "Measures submit kernel from 2 queues.";
    }
};
