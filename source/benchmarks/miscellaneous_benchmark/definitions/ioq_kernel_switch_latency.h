/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct IoqKernelSwitchLatencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    BooleanArgument useEvents;

    IoqKernelSwitchLatencyArguments()
        : kernelCount(*this, "kernelCount", "Count of kernels"),
          useEvents(*this, "useEvents", "Use events to synchronize between kernels") {}
};

struct IoqKernelSwitchLatency : TestCase<IoqKernelSwitchLatencyArguments> {
    using TestCase<IoqKernelSwitchLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "IoqKernelSwitchLatency";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next kernel for in order queue";
    }
};
