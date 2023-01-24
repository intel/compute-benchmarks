/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmCopyRegionArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ThreeComponentSizeArgument region;
    ThreeComponentOffsetArgument origin;
    ByteSizeArgument size;
    BufferContentsArgument contents;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    UsmCopyRegionArguments()
        : sourcePlacement(*this, "src", "Placement of the source buffer"),
          destinationPlacement(*this, "dst", "Placement of the destination buffer"),
          region(*this, "region", "Size of the region"),
          origin(*this, "origin", "Origin of the region"),
          size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffers"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct UsmCopyRegion : TestCase<UsmCopyRegionArguments> {
    using TestCase<UsmCopyRegionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyRegion";
    }

    std::string getHelp() const override {
        return "allocates two unified shared memory buffers and measures region copy bandwidth between them using immediate command list.";
    }
};
