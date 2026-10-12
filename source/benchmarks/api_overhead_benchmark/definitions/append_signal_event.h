/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendSignalEventArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;
    BooleanArgument counterBasedEvents;
    BooleanArgument timestampEvents;

    AppendSignalEventArguments()
        : appendCount(*this, "appendCount", "Number of appends to run"),
          counterBasedEvents(*this, "counterBasedEvents", "Use counter-based events instead of events from an event pool"),
          timestampEvents(*this, "timestampEvents", "Create events with kernel timestamp support") {}
};

struct AppendSignalEvent : TestCase<AppendSignalEventArguments> {
    using TestCase<AppendSignalEventArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendSignalEvent";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendSignalEvent on CPU.";
    }
};
