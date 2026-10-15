/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/mutable_command_flag_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListGetNextCommandIdArguments : TestCaseArgumentContainer {
    MutableCommandFlagArgument mutableCommandFlag;

    CommandListGetNextCommandIdArguments()
        : mutableCommandFlag(*this, "mutableCommandFlag", "Specifies the mutable command flag to be used in the test. "
                                                          "Supported values: All, KernelArguments, GroupCount, GroupSize, GlobalOffset, SignalEvent, WaitEvents, GraphArguments.") {}
};

struct CommandListGetNextCommandId : TestCase<CommandListGetNextCommandIdArguments> {
    using TestCase<CommandListGetNextCommandIdArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListGetNextCommandId";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListGetNextCommandIdExp call on CPU. "
               "Supported mutableCommandFlag values: All, KernelArguments, GroupCount, GroupSize, GlobalOffset, SignalEvent, WaitEvents, GraphArguments.";
    }
};