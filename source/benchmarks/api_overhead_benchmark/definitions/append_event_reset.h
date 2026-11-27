/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendEventResetArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;
    BooleanArgument timestampEvents;

    AppendEventResetArguments()
        : appendCount(*this, "appendCount", "Number of appends to run"),
          timestampEvents(*this, "timestampEvents", "Create events with kernel timestamp support") {}
};

struct AppendEventReset : TestCase<AppendEventResetArguments> {
    using TestCase<AppendEventResetArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendEventReset";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendEventReset on CPU.";
    }
};
