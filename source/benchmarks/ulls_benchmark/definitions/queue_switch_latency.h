/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct QueueSwitchArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelTime;
    PositiveIntegerArgument switchCount;

    QueueSwitchArguments()
        : kernelTime(*this, "kernelTime", "Time for each kernel execution"),
          switchCount(*this, "switchCount", "How many switches form each iteration") {}
};

struct QueueSwitch : TestCase<QueueSwitchArguments> {
    using TestCase<QueueSwitchArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "QueueSwitch";
    }

    std::string getHelp() const override {
        return "creates multiple queues, creates dependencies with events between those and measures switch time";
    }
};