/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitLinearKernelSizeArguments : TestCaseArgumentContainer {

    IntegerArgument kernelBatchSize;
    IntegerArgument kernelSize;
    BooleanArgument inOrderQueue;
    BooleanArgument useProfiling;

    KernelSubmitLinearKernelSizeArguments() : kernelBatchSize(*this, "kernelBatchSize", "Size of a batch of kernels after which synchronization occurs. 0 means one synchronization after all kernels are submitted."),
                                              kernelSize(*this, "kernelSize", "Size of the kernel (allowed: 32, 128, 512, 1024, 5120)."),
                                              inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
                                              useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property") {
        inOrderQueue = 0;
        useProfiling = 0;
    }
};

struct KernelSubmitLinearKernelSize : TestCase<KernelSubmitLinearKernelSizeArguments> {
    using TestCase<KernelSubmitLinearKernelSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitLinearKernelSize";
    }

    std::string getHelp() const override {
        return "Measures submit with linear kernel size.";
    }
};
