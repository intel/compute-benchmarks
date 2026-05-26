/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EventHostSynchronizeArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelExecutionTime;
    PositiveIntegerArgument batchSize;
    BooleanArgument useKernelTimestamps;
    BooleanArgument inOrderQueue;

    EventHostSynchronizeArguments()
        : kernelExecutionTime(*this, "kernelExecutionTime", "Approximately how long a single kernel executes, in us"),
          batchSize(*this, "batchSize", "Number of zeEventHostSynchronize calls measured per result"),
          useKernelTimestamps(*this, "useKernelTimestamps", "Use events with kernel timestamp support"),
          inOrderQueue(*this, "inOrderQueue", "Use an in-order immediate command list with counter-based events") {}
};

struct EventHostSynchronize : TestCase<EventHostSynchronizeArguments> {
    using TestCase<EventHostSynchronizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventHostSynchronize";
    }

    std::string getHelp() const override {
        return "Measures zeEventHostSynchronize latency, thread and process CPU time, and CPU utilization for work submitted to an immediate command list";
    }
};
