/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct KernelSwitchLatencyImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument barrier;
    BooleanArgument hostVisible;
    BooleanArgument inOrder;
    BooleanArgument counterBasedEvents;
    BooleanArgument useProfiling;

    KernelSwitchLatencyImmediateArguments()
        : kernelCount(*this, "count", "Count of kernels"),
          kernelExecutionTime(*this, "execTime", "Approximately how long a single kernel executes, in us"),
          barrier(*this, "barrier", "synchronization with barrier instead of events"),
          hostVisible(*this, "hostVisible", "events are with host visible flag"),
          inOrder(*this, "inOrder", "use in order queue/command list"),
          counterBasedEvents(*this, "counterBasedEvents", "use counter based events for in order"),
          useProfiling(*this, "useProfiling", "use profiling to obtain switch time") {}
};

struct KernelSwitchLatencyImmediate : TestCase<KernelSwitchLatencyImmediateArguments> {
    using TestCase<KernelSwitchLatencyImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSwitchLatencyImmediate";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next kernel using immediate command lists";
    }
};
