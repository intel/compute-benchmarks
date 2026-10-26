/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include "command_list_query_arguments.h"

struct CommandListGetContextHandle : TestCase<CommandListQueryArguments> {
    using TestCase<CommandListQueryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListGetContextHandle";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListGetContextHandle call on CPU.";
    }
};
