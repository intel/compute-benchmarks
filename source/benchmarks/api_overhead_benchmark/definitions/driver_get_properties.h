/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct DriverGetPropertiesArguments : TestCaseArgumentContainer {
    DriverGetPropertiesArguments() {}
};

struct DriverGetProperties : TestCase<DriverGetPropertiesArguments> {
    using TestCase<DriverGetPropertiesArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DriverGetProperties";
    }

    std::string getHelp() const override {
        return "measures time spent in zeDriverGetProperties call on CPU.";
    }
};
