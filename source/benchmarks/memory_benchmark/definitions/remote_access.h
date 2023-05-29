/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/test_case/test_case.h"

struct RemoteAccessMemoryArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument useEvents;
    FractionBaseArgument remoteFraction;
    PositiveIntegerArgument workItemPackSize;

    RemoteAccessMemoryArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          remoteFraction(*this, "remoteFraction", "Fraction of remote memory access. 1 / n"),
          workItemPackSize(*this, "workItemSize", "Number of work items group together for remote check") {}
};

struct RemoteAccessMemory : TestCase<RemoteAccessMemoryArguments> {
    using TestCase<RemoteAccessMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RemoteAccessMemory";
    }

    std::string getHelp() const override {
        return "Uses stream memory in a fashion described by 'type' to measure bandwidth with different"
               "percentages of remote memory access. Triad means two buffers are read and one is written to."
               "In read and write memory is only read or written to.";
    }
};
