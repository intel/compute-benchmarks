/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EventQueryProfilingDataArguments : TestCaseArgumentContainer {
    Uint32Argument eventCount;
    Uint32Argument splitCount;

    EventQueryProfilingDataArguments()
        : eventCount(*this, "eventCount", "Number of events to query profiling data from"),
          splitCount(*this, "splitCount", "Number of internal timestamp splits per operation (1, 2, or 3)") {}
};

struct EventQueryProfilingData : TestCase<EventQueryProfilingDataArguments> {
    using TestCase<EventQueryProfilingDataArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EventQueryProfilingData";
    }

    std::string getHelp() const override {
        return "measures time spent querying profiling data from events";
    }
};
