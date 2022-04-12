/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/test_case/test_case.h"

struct RemoteAccessArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument useEvents;
    FractionBaseArgument remoteFraction;
    PositiveIntegerArgument workItemPackSize;

    RemoteAccessArguments()
        : size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          remoteFraction(*this, "remoteFraction", "Fraction of remote memory access. 1 / n"),
          workItemPackSize(*this, "workItemSize", "Number of work items group together for remote check") {}
};

struct RemoteAccess : TestCase<RemoteAccessArguments> {
    using TestCase<RemoteAccessArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RemoteAccessMemory";
    }

    std::string getHelp() const override {
        return "Uses stream memory triad to measure bandwidth with different percentages of remote memory access.";
    }
};
