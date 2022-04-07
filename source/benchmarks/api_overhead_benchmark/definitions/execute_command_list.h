/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListArguments : TestCaseArgumentContainer {
    BooleanArgument useFence;
    BooleanArgument measureCompletionTime;

    ExecuteCommandListArguments()
        : useFence(*this, "UseFence", "Pass a non-null ze_fence_handle_t to the API call"),
          measureCompletionTime(*this, "measureCompletionTime", "Measures time taken to complete the submission (default is to measure only Execute call)") {}
};

struct ExecuteCommandList : TestCase<ExecuteCommandListArguments> {
    using TestCase<ExecuteCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandList";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandQueueExecuteCommandLists on CPU.";
    }
};
