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
    BooleanArgument useIoq;

    ExecuteCommandListImmediateArguments()
        : useProfiling(*this, "Profiling", "Pass a profiling ze_event_t to the API call"),
          amountOfCalls(*this, "CallsCount", "amount of calls that is being meassured"),
          measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submission (default is to measure only Immediate call)"),
          useBarrierSynchronization(*this, "BarrierSynchro", "Uses barrier synchronization instead of waiting for event from last kernel"),
          kernelExecutionTime(*this, "KernelExecTime", "How long a single kernel executes, in us"),
          useEventForHostSync(*this, "EventSync",
                              "If true, use events to synchronize with host. If false, use zeCommandListHostSynchronize"),
          useIoq(*this, "ioq", "Use In order queue") {}
};

struct ExecuteCommandListImmediate : TestCase<ExecuteCommandListImmediateArguments> {
    using TestCase<ExecuteCommandListImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in appending launch kernel for immediate command list on CPU.";
    }
};
