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
    BooleanArgument measureCompletionTime;

    KernelSubmitSlmSizeArguments() : kernelBatchSize(*this, "KernelBatchSize", "Sychronization interval."),
                                     slmNum(*this, "SlmNum", "Size of used shared local memory."),
                                     useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property. Currently, only in-order queue is supported."),
                                     measureCompletionTime(*this, "MeasureCompletionTime", "Measures total time of one batch submission (multiple submits) together with wait after the batch. Default is to measure one submit call only.") {}
};

struct KernelSubmitSlmSize : TestCase<KernelSubmitSlmSizeArguments> {

    std::string getTestCaseName() const override {
        return "KernelSubmitSlmSize";
    }

    std::string getHelp() const override {
        return "Measures submit with SLM size specified.";
    }
};
