/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct EmptyKernelArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    EmptyKernelArguments()
        : measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()),
          workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)") {}
};

class EmptyKernel : public TestCase<EmptyKernelArguments> {
  public:
    using TestCase<EmptyKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EmptyKernel";
    }

    std::string getHelp() const override {
        return "measures time required to run an empty kernel on GPU.";
    }
};
