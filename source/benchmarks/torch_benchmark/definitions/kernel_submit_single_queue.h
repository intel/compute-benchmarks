/*
 * Copyright (C) 2025 Intel Corporation
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
    DataTypeArgument KernelDataType;
    KernelNameArgument KernelName;
    Uint32Argument KernelParamsNum;
    NonNegativeIntegerArgument KernelBatchSize;
    KernelSubmitPatternArgument KernelSubmitPattern;
    Uint32Argument KernelWGCount;
    Uint32Argument KernelWGSize;

    KernelSubmitSingleQueueArguments()
        : KernelDataType(*this, "KernelDataType", "Data type for kernel operations"),
          KernelName(*this, "KernelName", "Name of the kernel to execute"),
          KernelParamsNum(*this, "KernelParamsNum", "Number of kernel parameters for add operation"),
          KernelBatchSize(*this, "KernelBatchSize", "Size of each batch of kernels before syncing. If 0, no batching is performed"),
          KernelSubmitPattern(*this, "KernelSubmitPattern", "Pattern for submitting kernels"),
          KernelWGCount(*this, "KernelWGCount", "Number of workgroups"),
          KernelWGSize(*this, "KernelWGSize", "Size of each workgroup") {}
};

struct KernelSubmitSingleQueue : TestCase<KernelSubmitSingleQueueArguments> {
    using TestCase<KernelSubmitSingleQueueArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitSingleQueue";
    }

    std::string getHelp() const override {
        return "measures time spent on executing simple math kernels";
    }
};
