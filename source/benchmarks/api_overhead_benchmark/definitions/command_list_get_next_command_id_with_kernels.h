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

struct CommandListGetNextCommandIdWithKernelsArguments : TestCaseArgumentContainer {
    MutableCommandFlagArgument mutableCommandFlag;
    PositiveIntegerArgument numKernels;

    CommandListGetNextCommandIdWithKernelsArguments()
        : mutableCommandFlag(*this, "mutableCommandFlag", "Specifies the mutable command flag to be used in the test. "
                                                          "Supported values: All, KernelArguments, GroupCount, GroupSize, GlobalOffset, SignalEvent, WaitEvents, KernelInstructions, GraphArguments."),
          numKernels(*this, "numKernels", "Number of kernels that user can switch between using zeCommandListUpdateMutableCommandKernelsExp call.") {}
};

struct CommandListGetNextCommandIdWithKernels : TestCase<CommandListGetNextCommandIdWithKernelsArguments> {
    using TestCase<CommandListGetNextCommandIdWithKernelsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CommandListGetNextCommandIdWithKernels";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListGetNextCommandIdWithKernelsExp call on CPU. "
               "Supported mutableCommandFlag values: All, KernelArguments, GroupCount, GroupSize, GlobalOffset, SignalEvent, WaitEvents, KernelInstructions, GraphArguments.";
    }
};