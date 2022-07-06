/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include "execute_command_list.h"

struct ExecuteCommandListForCopyEngine : TestCase<ExecuteCommandListArguments> {
    using TestCase<ExecuteCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListForCopyEngine";
    }

    std::string getHelp() const override {
        return "measures CPU time spent in zeCommandQueueExecuteCommandLists for copy-only path";
    }
};
