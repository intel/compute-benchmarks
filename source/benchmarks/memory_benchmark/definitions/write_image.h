/*
 * Copyright (C) 2024 Intel Corporation
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

struct WriteImageArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument forceBlitter;
    HostptrBufferReuseModeArgument hostPtrPlacement;
    BooleanArgument useEvents;

    WriteImageArguments()
        : size(*this, "size", "Size of the image"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          hostPtrPlacement(*this, "ptrPlacement", "memory placement of host_ptr passed to WriteImage call"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct WriteImage : TestCase<WriteImageArguments> {
    using TestCase<WriteImageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WriteImage";
    }

    std::string getHelp() const override {
        return "Measures time spent during Write Image calls ";
    }
};
