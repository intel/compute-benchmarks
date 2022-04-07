/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendLaunchKernelArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    IntegerArgument workgroupSize;
    BooleanArgument useEvent;

    AppendLaunchKernelArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size, pass 0 to make the driver calculate it during enqueue"),
          useEvent(*this, "event", "Pass output event to the enqueue call") {}
};

struct AppendLaunchKernel : TestCase<AppendLaunchKernelArguments> {
    using TestCase<AppendLaunchKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendLaunchKernel";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendLaunchKernel on CPU.";
    }
};
