/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/event_scope_argument.h"
#include "framework/test_case/test_case.h"

struct InOrderWaitAppendArguments : TestCaseArgumentContainer {
    BooleanArgument counterBasedEvents;
    BooleanArgument isCompleted;
    PositiveIntegerArgument kernelExecutionTime;

    InOrderWaitAppendArguments() : counterBasedEvents(*this, "counterBasedEvents", "Use Counter Based Events"),
                                   isCompleted(*this, "isCompleted", "Is the event completed before appending"),
                                   kernelExecutionTime(*this, "kernelExecutionTime", "How long a single kernel executes, in us in non-completed mode") {}
};

struct InOrderWaitAppend : TestCase<InOrderWaitAppendArguments> {
    using TestCase<InOrderWaitAppendArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "InOrderWaitAppend";
    }

    std::string getHelp() const override {
        return "Measures time spent to append wait on event command to in-order command list.";
    }
};
