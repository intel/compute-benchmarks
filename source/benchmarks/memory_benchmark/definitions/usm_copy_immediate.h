/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmCopyImmediateArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    BufferContentsArgument contents;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;
    BooleanArgument withCopyOffload;

    UsmCopyImmediateArguments()
        : sourcePlacement(*this, "src", "Placement of the source buffer"),
          destinationPlacement(*this, "dst", "Placement of the destination buffer"),
          size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffers"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}
};

struct UsmCopyImmediate : TestCase<UsmCopyImmediateArguments> {
    using TestCase<UsmCopyImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyImmediate";
    }

    std::string getHelp() const override {
        return "allocates two unified shared memory buffers and measures copy bandwidth between them using immediate command list.";
    }
};
