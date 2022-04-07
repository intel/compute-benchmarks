/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct CopyEntireImageArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    CopyEntireImageArguments()
        : size(*this, "size", "Size of the image"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct CopyEntireImage : TestCase<CopyEntireImageArguments> {
    using TestCase<CopyEntireImageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyEntireImage";
    }

    std::string getHelp() const override {
        return "allocates two image objects and measures copy bandwidth between them. Images will "
               "be placed in device memory, if it's available.";
    }
};
