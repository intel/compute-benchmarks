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
          kernelExecutionTime(*this, "kTime", "Approximately how long a single kernel executes, in us"),
          barrier(*this, "barrier", "synchronization with barrier instead of events"),
          hostVisible(*this, "cpuVisible", "events are with host visible flag"),
          inOrder(*this, "ioq", "use in order queue/command list"),
          counterBasedEvents(*this, "ctrEvents", "use counter based events for in order"),
          useProfiling(*this, "profiling", "use profiling to obtain switch time") {}
};

struct KernelSwitchLatencyImmediate : TestCase<KernelSwitchLatencyImmediateArguments> {
    using TestCase<KernelSwitchLatencyImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSwitchImm";
    }

    std::string getHelp() const override {
        return "measures time from end of one kernel till start of next kernel using immediate command lists";
    }
};
