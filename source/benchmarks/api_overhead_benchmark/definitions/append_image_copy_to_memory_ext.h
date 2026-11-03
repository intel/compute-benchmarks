/*
 * Copyright (C) 2026 Intel Corporation
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

struct AppendImageCopyToMemoryExtArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument wholeImage;
    UsmMemoryPlacementArgument dst;
    BooleanArgument useEvent;
    PositiveIntegerArgument appendCount;
    BooleanArgument forceBlitter;

    AppendImageCopyToMemoryExtArguments()
        : size(*this, "size", "Size of the source image"),
          wholeImage(*this, "wholeImage", "Copy the whole image by passing no source region, as opposed to a region of a single pixel"),
          dst(*this, "dst", "Placement of destination memory"),
          useEvent(*this, "event", "Pass output event to the append call"),
          appendCount(*this, "appendCount", "Number of appends to run"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct AppendImageCopyToMemoryExt : TestCase<AppendImageCopyToMemoryExtArguments> {
    using TestCase<AppendImageCopyToMemoryExtArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendImageCopyToMemoryExt";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendImageCopyToMemoryExt on CPU. "
               "The source is an RGBA float image and the destination row and slice pitches are 0, so the driver computes them.";
    }
};
