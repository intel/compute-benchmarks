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

struct AppendWaitOnEventsImmediateArguments : TestCaseArgumentContainer {
    BooleanArgument eventSignaled;

    AppendWaitOnEventsImmediateArguments() : eventSignaled(*this, "eventSignaled", "Event is already signaled before zeCommandListAppendWaitOnEvents call") {}
};

struct AppendWaitOnEventsImmediate : TestCase<AppendWaitOnEventsImmediateArguments> {
    using TestCase<AppendWaitOnEventsImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendWaitOnEventsImmediate";
    }

    std::string getHelp() const override {
        return "Measures time spent to zeCommandListAppendWaitOnEvents using immediate command list.";
    }
};
