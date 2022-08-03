/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct DriverGetArguments : TestCaseArgumentContainer {
    BooleanArgument getDriverCount;

    DriverGetArguments()
        : getDriverCount(*this, "getDriverCount", "Whether to measure driver count or driver get") {}
};

struct DriverGet : TestCase<DriverGetArguments> {
    using TestCase<DriverGetArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DriverGet";
    }

    std::string getHelp() const override {
        return "measures time spent in driver get call on CPU.";
    }
};
