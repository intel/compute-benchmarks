/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct DriverGetApiVersionArguments : TestCaseArgumentContainer {
    DriverGetApiVersionArguments() {}
};

struct DriverGetApiVersion : TestCase<DriverGetApiVersionArguments> {
    using TestCase<DriverGetApiVersionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DriverGetApiVersion";
    }

    std::string getHelp() const override {
        return "measures time spent in zeDriverGetApiVersion call on CPU.";
    }
};
