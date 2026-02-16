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

struct EventDestroyArguments : TestCaseArgumentContainer {
    BooleanArgument useProfiling;
    BooleanArgument hostVisible;
    EventScopeArgument signalScope;
    EventScopeArgument waitScope;
    Uint32Argument eventCount;
    BooleanArgument counterBasedEvents;

    EventDestroyArguments()
        : useProfiling(*this, "useProfiling", "Event will use profiling"),
          hostVisible(*this, "hostVisible", "Event will set host visible flag"),
          signalScope(*this, "signal", "Type of signal scope"),
          waitScope(*this, "wait", "Type of wait scope"),
          eventCount(*this, "eventCount", "Number of events to create"),
          counterBasedEvents(*this, "useCounterBasedEvents", "Event will be of counter based type") {}
};

struct EventDestroy : TestCase<EventDestroyArguments> {
    using TestCase<EventDestroyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventDestroy";
    }

    std::string getHelp() const override {
        return "measures time spent to destroy completed event";
    }
};