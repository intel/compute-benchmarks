/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListWithFenceDestroyArguments : TestCaseArgumentContainer {
    ExecuteCommandListWithFenceDestroyArguments() {}
};

struct ExecuteCommandListWithFenceDestroy : TestCase<ExecuteCommandListWithFenceDestroyArguments> {
    using TestCase<ExecuteCommandListWithFenceDestroyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListWithFenceDestroy";
    }

    std::string getHelp() const override {
        return "measures time spent in zeFenceDestroy on CPU when fences are used.";
    }
};
