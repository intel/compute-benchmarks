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

struct AppendImageCopyFromMemoryExtArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument wholeImage;
    UsmMemoryPlacementArgument src;
    BooleanArgument useEvent;
    PositiveIntegerArgument appendCount;
    BooleanArgument forceBlitter;

    AppendImageCopyFromMemoryExtArguments()
        : size(*this, "size", "Size of the destination image"),
          wholeImage(*this, "wholeImage", "Copy the whole image by passing no destination region, as opposed to a region of a single pixel"),
          src(*this, "src", "Placement of source memory"),
          useEvent(*this, "event", "Pass output event to the append call"),
          appendCount(*this, "appendCount", "Number of appends to run"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct AppendImageCopyFromMemoryExt : TestCase<AppendImageCopyFromMemoryExtArguments> {
    using TestCase<AppendImageCopyFromMemoryExtArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendImageCopyFromMemoryExt";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendImageCopyFromMemoryExt on CPU. "
               "The destination is an RGBA float image and the source row and slice pitches are 0, so the driver computes them.";
    }
};
