/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelAndCopyPipelineImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument stages;
    ByteSizeArgument size;
    PositiveIntegerArgument kernelTime;
    BooleanArgument withCopyOffload;
    BooleanArgument signalAfterCopy;

    KernelAndCopyPipelineImmediateArguments()
        : stages(*this, "stages", "Number of pipeline stages per iteration. Each stage is one kernel followed by one device to host copy"),
          size(*this, "size", "Size of the device to host copy of each stage"),
          kernelTime(*this, "kernelTime", "Number of operations of the single work-item kernel of each stage"),
          withCopyOffload(*this, "withCopyOffload", "If true, the copies go to a second in-order immediate command list on the compute engine created with the copy offload hint. If false, they go to the command list of the kernels"),
          signalAfterCopy(*this, "signalAfterCopy", "If true, the copy command list appends a signal of a counter-based event right after each copy") {}
};

struct KernelAndCopyPipelineImmediate : TestCase<KernelAndCopyPipelineImmediateArguments> {
    using TestCase<KernelAndCopyPipelineImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelAndCopyPipelineImmediate";
    }

    std::string getHelp() const override {
        return "runs a pipeline on in-order immediate command lists with counter-based events: each stage launches a kernel and then copies "
               "a device buffer to host memory, the copy waiting for the kernel of its stage. Measures the time from the first append until "
               "all command lists are synchronized. With withCopyOffload the copy of a stage can run on a copy engine during the kernel of "
               "the next stage, so the time is close to the longer of all kernels and all copies instead of their sum.";
    }
};
