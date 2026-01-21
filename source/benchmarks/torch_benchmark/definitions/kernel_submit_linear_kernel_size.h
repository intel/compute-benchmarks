/*
 * Copyright (C) 2025-2026 Intel Corporation
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

    KernelSubmitLinearKernelSizeArguments() : kernelBatchSize(*this, "kernelBatchSize", "Size of a batch of kernels after which synchronization occurs. 0 means one synchronization after all kernels are submitted."),
                                              kernelSize(*this, "kernelSize", "Size of the kernel (allowed: 32, 128, 512, 1024, 5120).") {}
};

struct KernelSubmitLinearKernelSize : TestCase<KernelSubmitLinearKernelSizeArguments> {

    std::string getTestCaseName() const override {
        return "KernelSubmitLinearKernelSize";
    }

    std::string getHelp() const override {
        return "Measures submit with linear kernel size.";
    }
};
