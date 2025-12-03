/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitSlmSizeArguments : TestCaseArgumentContainer {

    IntegerArgument batchSize;
    IntegerArgument slmNum;
    IntegerArgument warmupIterations;

    KernelSubmitSlmSizeArguments() : batchSize(*this, "batchSize", "Sychronization interval."),
                                     slmNum(*this, "slmNum", "Size of used shared local memory."),
                                     warmupIterations(*this, "warmupIterations", "Number of warmup iterations.") {}
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
