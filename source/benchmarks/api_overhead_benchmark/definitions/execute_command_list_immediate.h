/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListImmediateArguments : TestCaseArgumentContainer {
    BooleanArgument useProfiling;
    PositiveIntegerArgument amountOfCalls;
    BooleanArgument measureCompletionTime;
    BooleanArgument useBarrierSynchronization;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument useEventForHostSync;

    ExecuteCommandListImmediateArguments()
        : useProfiling(*this, "UseProfiling", "Pass a profiling ze_event_t to the API call"),
          amountOfCalls(*this, "CallsCount", "amount of calls that is being meassured"),
          measureCompletionTime(*this, "MeasureCompletionTime", "Measures time taken to complete the submission (default is to measure only Immediate call)"),
          useBarrierSynchronization(*this, "useBarrierSynchronization", "Uses barrier synchronization instead of waiting for event from last kernel"),
          kernelExecutionTime(*this, "KernelExecutionTime", "How long a single kernel executes, in us"),
          useEventForHostSync(*this, "UseEventForHostSync",
                              "If true, use events to synchronize with host. If false, use zeCommandListHostSynchronize") {}
};

struct ExecuteCommandListImmediate : TestCase<ExecuteCommandListImmediateArguments> {
    using TestCase<ExecuteCommandListImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in appending launch kernel for immediate command list on CPU.";
    }
};
