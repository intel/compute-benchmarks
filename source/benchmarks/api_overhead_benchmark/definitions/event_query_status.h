/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/event_scope_argument.h"
#include "framework/test_case/test_case.h"

struct EventQueryStatusArguments : TestCaseArgumentContainer {
    BooleanArgument eventSignaled;

    EventQueryStatusArguments() : eventSignaled(*this, "eventSignaled", "Event will be set as signaled") {}
};

struct EventQueryStatus : TestCase<EventQueryStatusArguments> {
    using TestCase<EventQueryStatusArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventQueryStatus";
    }

    std::string getHelp() const override {
        return "Measures time spent to query event status";
    }
};
