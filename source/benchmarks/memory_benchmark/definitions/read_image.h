/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/hostptr_reuse_mode_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct ReadImageArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument forceBlitter;
    HostptrBufferReuseModeArgument hostPtrPlacement;
    BooleanArgument useEvents;
    IntegerArgument hostPtrAlignment;

    ReadImageArguments()
        : size(*this, "size", "Size of the image"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          hostPtrPlacement(*this, "ptrPlacement", "memory placement of host_ptr passed to ReadImage call"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          hostPtrAlignment(*this, "ptrAlignment", "host pointer alignment") {}
};

struct ReadImage : TestCase<ReadImageArguments> {
    using TestCase<ReadImageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ReadImage";
    }

    std::string getHelp() const override {
        return "Measures time spent during Read Image calls ";
    }
};
