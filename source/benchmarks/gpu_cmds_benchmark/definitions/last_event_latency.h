/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct LastEventLatencyArguments : TestCaseArgumentContainer {
    BooleanArgument signalOnBarrier;

    LastEventLatencyArguments() : signalOnBarrier(*this, "signalOnBarrier", "Place signal event on barrier instead of launched kernel.") {}
};

struct LastEventLatency : TestCase<LastEventLatencyArguments> {

    using TestCase<LastEventLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "LastEventLatency";
    }

    std::string getHelp() const override {
        return "Measures different ways to retrieve event from last submission.";
    }
};
