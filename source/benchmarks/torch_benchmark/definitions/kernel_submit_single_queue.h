/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/argument/enum/kernel_name_argument.h"
#include "framework/argument/enum/kernel_submit_pattern_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitSingleQueueArguments : TestCaseArgumentContainer {
    DataTypeArgument kernelDataType;
    KernelNameArgument kernelName;
    Uint32Argument kernelParamsNum;
    NonNegativeIntegerArgument kernelBatchSize;
    KernelSubmitPatternArgument kernelSubmitPattern;
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    BooleanArgument useProfiling;
    BooleanArgument useEvents;

    KernelSubmitSingleQueueArguments()
        : kernelDataType(*this, "KernelDataType", "Data type for kernel operations"),
          kernelName(*this, "KernelName", "Name of the kernel to execute"),
          kernelParamsNum(*this, "KernelParamsNum", "Number of kernel parameters for add operation"),
          kernelBatchSize(*this, "KernelBatchSize", "Size of each batch of kernels before syncing. If 0, no batching is performed"),
          kernelSubmitPattern(*this, "KernelSubmitPattern", "Pattern for submitting kernels"),
          kernelWGCount(*this, "KernelWGCount", "Number of workgroups"),
          kernelWGSize(*this, "KernelWGSize", "Size of each workgroup"),
          useProfiling(*this, "Profiling", "Create the queue with the enable_profiling property"),
          useEvents(*this, "UseEvents", "Use events when enqueuing kernels") {}
};

struct KernelSubmitSingleQueue : TestCase<KernelSubmitSingleQueueArguments> {
    using TestCase<KernelSubmitSingleQueueArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitSingleQueue";
    }

    std::string getHelp() const override {
        return "Measures time spent on executing simple math kernels.";
    }
};
