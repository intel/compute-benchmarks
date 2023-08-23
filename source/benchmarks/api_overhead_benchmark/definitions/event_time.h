/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/event_scope_argument.h"
#include "framework/test_case/test_case.h"

struct EventTimeArguments : TestCaseArgumentContainer {
    BooleanArgument useProfiling;
    BooleanArgument hostVisible;
    EventScopeArgument signalScope;
    EventScopeArgument waitScope;
    Uint32Argument eventCount;

    EventTimeArguments()
        : useProfiling(*this, "useProfiling", "Event will use profiling"),
          hostVisible(*this, "hostVisible", "Event will set host visible flag"),
          signalScope(*this, "signal", "Type of signal scope"),
          waitScope(*this, "wait", "Type of wait scope"),
          eventCount(*this, "eventCount", "Number of events to create") {}
};

struct EventTime : TestCase<EventTimeArguments> {
    using TestCase<EventTimeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventCreation";
    }

    std::string getHelp() const override {
        return "measures time spent to create event";
    }
};
