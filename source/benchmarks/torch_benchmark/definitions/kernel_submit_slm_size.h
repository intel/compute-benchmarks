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

    PositiveIntegerArgument kernelBatchSize;
    IntegerArgument slmNum;
    BooleanArgument useProfiling;
    BooleanArgument measureCompletion;

    KernelSubmitSlmSizeArguments() : kernelBatchSize(*this, "kernelBatchSize", "Sychronization interval."),
                                     slmNum(*this, "slmNum", "Size of used shared local memory."),
                                     useProfiling(*this, "useProfiling", "Create the queue with the enable_profiling property"),
                                     measureCompletion(*this, "measureCompletion", "Measures total time of one batch submission (multiple submits) together with wait after the batch. Default is to measure one submit call only.") {}
};

struct KernelSubmitSlmSize : TestCase<KernelSubmitSlmSizeArguments> {

    std::string getTestCaseName() const override {
        return "KernelSubmitSlmSize";
    }

    std::string getHelp() const override {
        return "Measures submit with SLM size specified.";
    }
};
