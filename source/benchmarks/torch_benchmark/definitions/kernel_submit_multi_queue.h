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

    IntegerArgument kernelWGCount;
    IntegerArgument kernelWGSize;
    IntegerArgument kernelsPerQueue;

    KernelSubmitMultiQueueArguments() : kernelWGCount(*this, "kernelWGCount", "Number of workgroups."),
                                        kernelWGSize(*this, "kernelWGSize", "Size of each workgroup."),
                                        kernelsPerQueue(*this, "kernelsPerQueue", "Number of kernels per queue.") {}
};

struct KernelSubmitMultiQueue : TestCase<KernelSubmitMultiQueueArguments> {
    using TestCase<KernelSubmitMultiQueueArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitMultiQueue";
    }

    std::string getHelp() const override {
        return "Measures submit kernel from 2 queues.";
    }
};
