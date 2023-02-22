/*
 * Copyright (C) 2023 Intel Corporation
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

struct CopyImageRegionArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    CopyImageRegionArguments()
        : size(*this, "size", "Size of the image"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct CopyImageRegion : TestCase<CopyImageRegionArguments> {
    using TestCase<CopyImageRegionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyImageRegion";
    }

    std::string getHelp() const override {
        return "allocates two image objects and measures region copy bandwidth between them using immediate command list for Level Zero and command queue for OpenCL.";
    }
};
