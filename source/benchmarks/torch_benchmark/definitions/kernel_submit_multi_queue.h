/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitMultiQueueArguments : TestCaseArgumentContainer {

    IntegerArgument workgroupCount;
    IntegerArgument workgroupSize;
    IntegerArgument kernelsPerQueue;

    KernelSubmitMultiQueueArguments() : workgroupCount(*this, "workgroupCount", "Number of workgroups."),
                                        workgroupSize(*this, "workgroupSize", "Size of workgroup."),
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
