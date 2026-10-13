/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct InOrderWaitlistAppendArguments : TestCaseArgumentContainer {
    BooleanArgument counterBasedEvents;
    BooleanArgument isCompleted;
    PositiveIntegerArgument kernelExecutionTime;

    InOrderWaitlistAppendArguments()
        : counterBasedEvents(*this, "counterBasedEvents", "Use Counter Based Events"),
          isCompleted(*this, "isCompleted", "Is the event completed before appending"),
          kernelExecutionTime(*this, "kernelExecutionTime", "How long the signaling kernel executes, in us in non-completed mode") {}
};

struct InOrderWaitlistAppend : TestCase<InOrderWaitlistAppendArguments> {
    using TestCase<InOrderWaitlistAppendArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "InOrderWaitlistAppend";
    }

    std::string getHelp() const override {
        return "Measures time spent to append a kernel to an in-order command list, when an event signaled by "
               "the very same command list is passed on the wait list of that append.";
    }
};
