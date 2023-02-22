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

struct CopyImageToBufferArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    ThreeComponentSizeArgument region;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    CopyImageToBufferArguments()
        : destinationPlacement(*this, "dst", "Placement of the destination buffer"),
          size(*this, "size", "Size of the buffer"),
          region(*this, "region", "Size of the source image region"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct CopyImageToBuffer : TestCase<CopyImageToBufferArguments> {
    using TestCase<CopyImageToBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyImageToBuffer";
    }

    std::string getHelp() const override {
        return "allocates image and buffer and measures copy bandwidth between them using immediate command list for Level Zero and command queue for OpenCL.";
    }
};
