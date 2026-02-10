/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct MultiArgumentKernelSwitchLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument workgroupSize;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument argumentCount;
    BooleanArgument profiling;

    MultiArgumentKernelSwitchLatencyArguments()
        : kernelCount(*this, "kernelCount", "Count of kernels"),
          workgroupSize(*this, "workgroupSize", "Size of the workgroup"),
          workgroupCount(*this, "workgroupCount", "Count of workgroups"),
          argumentCount(*this, "argumentCount", "Argument count in a kernel - supported values: 1, 4, 8, 16, 32, 64"),
          profiling(*this, "profiling", "Use profiling on the gpu to collect score") {}
};

struct MultiArgumentKernelSwitchLatency : TestCase<MultiArgumentKernelSwitchLatencyArguments> {
    using TestCase<MultiArgumentKernelSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiArgumentKernelSwitchLatency";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next kernel with different argument patterns";
    }
};
