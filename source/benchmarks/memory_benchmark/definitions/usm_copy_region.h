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
    UsmMemoryPlacementArgument dstptr;
    ThreeComponentSizeArgument dstRegion;
    ThreeComponentOffsetArgument dstOrigin;
    UsmMemoryPlacementArgument srcptr;
    ThreeComponentSizeArgument srcRegion;
    ThreeComponentOffsetArgument srcOrigin;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;
    BooleanArgument reuseCommandList;

    UsmCopyRegionArguments()
        : dstptr(*this, "dstptr", "Placement of the destination buffer"),
          dstRegion(*this, "dstRegion", ""),
          dstOrigin(*this, "dstOrigin", ""),
          srcptr(*this, "srcptr", "Placement of the source buffer"),
          srcRegion(*this, "srcRegion", ""),
          srcOrigin(*this, "srcOrigin", ""),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          reuseCommandList(*this, "reuseCmdList", "Command list is reused between iterations") {}
};

struct UsmCopyRegion : TestCase<UsmCopyRegionArguments> {
    using TestCase<UsmCopyRegionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyRegion";
    }

    std::string getHelp() const override {
        return "TODO";
    }
};
