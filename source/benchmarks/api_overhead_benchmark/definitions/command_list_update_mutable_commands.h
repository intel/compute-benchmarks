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

struct CommandListUpdateMutableCommandsArguments : TestCaseArgumentContainer {
    MutableCommandFlagArgument mutableCommandFlag;

    CommandListUpdateMutableCommandsArguments()
        : mutableCommandFlag(*this, "mutableCommandFlag", "Specifies which mutation type(s) to apply via zeCommandListUpdateMutableCommandsExp. "
                                                          "Supported values: KernelArguments, GroupCount, GroupSize, GlobalOffset, All. "
                                                          "'All' chains kernelArgs + groupCount + groupSize + globalOffset for the single mutable command.") {}
};

struct CommandListUpdateMutableCommands : TestCase<CommandListUpdateMutableCommandsArguments> {
    using TestCase<CommandListUpdateMutableCommandsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListUpdateMutableCommands";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListUpdateMutableCommandsExp call on CPU. "
               "Mutates a single previously-appended kernel command via mutation descriptors chained on pNext. "
               "Supported mutableCommandFlag values: KernelArguments, GroupCount, GroupSize, GlobalOffset, All.";
    }
};
