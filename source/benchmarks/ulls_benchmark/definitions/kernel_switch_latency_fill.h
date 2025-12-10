/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct KernelSwitchLatencyFillArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    ByteSizeArgument fillSize;
    BooleanArgument inOrder;

    KernelSwitchLatencyFillArguments()
        : kernelCount(*this, "count", "Count of kernels"),
          fillSize(*this, "fillSize", "Size of the fill"),
          inOrder(*this, "ioq", "Use implicit dependency between kernels") {}
};

struct KernelSwitchLatencyFill : TestCase<KernelSwitchLatencyFillArguments> {
    using TestCase<KernelSwitchLatencyFillArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSwitchFill";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next fill kernel";
    }
};