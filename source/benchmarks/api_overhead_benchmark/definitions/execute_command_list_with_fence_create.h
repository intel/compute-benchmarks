/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListWithFenceCreateArguments : TestCaseArgumentContainer {
    ExecuteCommandListWithFenceCreateArguments() {}
};

struct ExecuteCommandListWithFenceCreate : TestCase<ExecuteCommandListWithFenceCreateArguments> {
    using TestCase<ExecuteCommandListWithFenceCreateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListWithFenceCreate";
    }

    std::string getHelp() const override {
        return "measures time spent in zeFenceCreate on CPU when fences are used.";
    }
};
