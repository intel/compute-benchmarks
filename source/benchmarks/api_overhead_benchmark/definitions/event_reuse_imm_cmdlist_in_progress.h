/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EventReuseImmCmdlistInProgressArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelExecutionTime;

    EventReuseImmCmdlistInProgressArguments()
        : kernelExecutionTime(*this, "kernelExecutionTime", "Approximately how long the stall kernel executes, in us") {}

    bool validateArgumentsExtra() const override {
        return this->kernelExecutionTime > 1;
    }
};

struct EventReuseImmCmdlistInProgress : TestCase<EventReuseImmCmdlistInProgressArguments> {
    using TestCase<EventReuseImmCmdlistInProgressArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventReuseImmCmdlistInProgress";
    }

    std::string getHelp() const override {
        return "measures CPU-side time of zeCommandListAppendLaunchKernel when reusing a signal "
               "event on an immediate command list that is currently in progress.";
    }
};
