/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/kernel_name_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitEventRecordWaitArguments : TestCaseArgumentContainer {
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    BooleanArgument useProfiling;

    KernelSubmitEventRecordWaitArguments() : kernelWGCount(*this, "KernelWGCount", "Number of workgroups."),
                                             kernelWGSize(*this, "KernelWGSize", "Size of each workgroup."),
                                             useProfiling(*this, "Profiling", "Create queues with profiling enabled.") {}
};

struct KernelSubmitEventRecordWait : TestCase<KernelSubmitEventRecordWaitArguments> {
    using TestCase<KernelSubmitEventRecordWaitArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitEventRecordWait";
    }

    std::string getHelp() const override {
        return "Measures the time taken to record an event for a submitted kernel, and waiting for this kernel to complete.";
    }
};
