/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/test_case/test_case.h"

struct RemoteAccessMaxSaturationArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument useEvents;
    FractionBaseArgument remoteFraction;
    PositiveIntegerArgument workItemPackSize;
    PositiveIntegerArgument writesPerWorkgroup;

    RemoteAccessMaxSaturationArguments()
        : size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          remoteFraction(*this, "remoteFraction", "Fraction of remote memory access. 1 / n"),
          workItemPackSize(*this, "workItemSize", "Number of work items group together for remote check"),
          writesPerWorkgroup(*this, "writesPerWorkgroup", "Number of work items per workgroup that access memory") {}
};

struct RemoteAccessMaxSaturation : TestCase<RemoteAccessMaxSaturationArguments> {
    using TestCase<RemoteAccessMaxSaturationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RemoteAccessMemoryMaxSaturation";
    }

    std::string getHelp() const override {
        return "Uses stream memory write to measure max data bus saturation with different percentages of remote memory access";
    }
};
