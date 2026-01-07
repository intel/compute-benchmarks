/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitSlmSizeArguments : TestCaseArgumentContainer {

    IntegerArgument kernelBatchSize;
    IntegerArgument slmNum;

    KernelSubmitSlmSizeArguments() : kernelBatchSize(*this, "kernelBatchSize", "Sychronization interval."),
                                     slmNum(*this, "slmNum", "Size of used shared local memory.") {}
};

struct KernelSubmitSlmSize : TestCase<KernelSubmitSlmSizeArguments> {
    using TestCase<KernelSubmitSlmSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitSlmSize";
    }

    std::string getHelp() const override {
        return "Measures submit with SLM size specified.";
    }
};
