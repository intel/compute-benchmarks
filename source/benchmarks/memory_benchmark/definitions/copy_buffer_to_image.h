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

struct CopyBufferToImageArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument region;
    UsmMemoryPlacementArgument sourcePlacement;
    ByteSizeArgument size;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    CopyBufferToImageArguments()
        : region(*this, "region", "Size of the destination image region"),
          sourcePlacement(*this, "src", "Placement of the source buffer"),
          size(*this, "size", "Size of the buffer"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct CopyBufferToImage : TestCase<CopyBufferToImageArguments> {
    using TestCase<CopyBufferToImageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyBufferToImage";
    }

    std::string getHelp() const override {
        return "allocates buffer and image and measures copy bandwidth between them using immediate command list for Level Zero and command queue for OpenCL.";
    }
};
