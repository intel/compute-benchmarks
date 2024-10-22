/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteRegularCommandListWithImmediateArguments : TestCaseArgumentContainer {
    BooleanArgument useEvent;
    BooleanArgument useProfiling;
    BooleanArgument measureCompletionTime;
    BooleanArgument inOrder;
    BooleanArgument counterBasedEvents;
    BooleanArgument waitEvent;

    ExecuteRegularCommandListWithImmediateArguments()
        : useEvent(*this, "event", "Pass output event to the enqueue call "),
          useProfiling(*this, "useProfiling", "Event will use profiling "),
          measureCompletionTime(*this, "measureCompletionTime", "Measures time taken to complete the submission "),
          inOrder(*this, "ioq", "use in order queue/command list"),
          counterBasedEvents(*this, "ctrBasedEvents", "use counter based events for in order"),
          waitEvent(*this, "waitEvent", "use preceeding operation to wait on") {}
};

struct ExecuteRegularCommandListWithImmediate : TestCase<ExecuteRegularCommandListWithImmediateArguments> {
    using TestCase<ExecuteRegularCommandListWithImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteRegularCommandListWithImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListImmediateAppendCommandListsExp on CPU.";
    }
};
