/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct UniqueKernelSwitchLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument workgroupSize;
    PositiveIntegerArgument workgroupCount;
    BooleanArgument profiling;

    UniqueKernelSwitchLatencyArguments()
        : kernelCount(*this, "kernelCount", "Count of unique kernel modules to execute, minimum 2"),
          workgroupSize(*this, "workgroupSize", "Size of the workgroup"),
          workgroupCount(*this, "workgroupCount", "Count of workgroups"),
          profiling(*this, "profiling", "Use profiling on the gpu to collect score") {}
};

struct UniqueKernelSwitchLatency : TestCase<UniqueKernelSwitchLatencyArguments> {
    using TestCase<UniqueKernelSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UniqueKernelSwitchLatency";
    }

    std::string getHelp() const override {
        return "measures time to switch between unique kernels in a command list";
    }
};
