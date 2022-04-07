/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListWithFenceUsageArguments : TestCaseArgumentContainer {
    ExecuteCommandListWithFenceUsageArguments() {}
};

struct ExecuteCommandListWithFenceUsage : TestCase<ExecuteCommandListWithFenceUsageArguments> {
    using TestCase<ExecuteCommandListWithFenceUsageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListWithFenceUsage";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandQueueExecuteCommandLists and zeFenceSynchronize on CPU when fences are used.";
    }
};
