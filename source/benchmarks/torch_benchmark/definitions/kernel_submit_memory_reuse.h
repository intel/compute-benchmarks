/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitMemoryReuseArguments : TestCaseArgumentContainer {

    Uint32Argument kernelBatchSize;
    DataTypeArgument kernelDataType;
    BooleanArgument useEvents;

    KernelSubmitMemoryReuseArguments() : kernelBatchSize(*this, "kernelBatchSize", "Size of a batch of kernels after which synchronization occurs. 0 means one synchronization after all kernels are submitted."),
                                         kernelDataType(*this, "kernelDataType", "Data type passed to kernel."),
                                         useEvents(*this, "UseEvents", "Use events when enqueuing kernels. When false, SYCL will use eventless enqueue functions.") {}
};

struct KernelSubmitMemoryReuse : TestCase<KernelSubmitMemoryReuseArguments> {
    using TestCase<KernelSubmitMemoryReuseArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitMemoryReuse";
    }

    std::string getHelp() const override {
        return "Measures submit kernel with memory reuse.";
    }
};
