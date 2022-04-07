/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/long_hex_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmFillSpecificPatternArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument usmMemoryPlacement;
    ByteSizeArgument bufferSize;
    BufferContentsArgument contents;
    LongHexArgument pattern;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    UsmFillSpecificPatternArguments()
        : usmMemoryPlacement(*this, "memory", "Placement of the buffer"),
          bufferSize(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          pattern(*this, "pattern", "The fill pattern represented hexadecimally, e.g. 0x91ABCD1254"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct UsmFillSpecificPattern : TestCase<UsmFillSpecificPatternArguments> {
    using TestCase<UsmFillSpecificPatternArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmFillSpecificPattern";
    }

    std::string getHelp() const override {
        return "allocates a unified memory buffer and measures fill bandwidth. Allow specifying arbitrary pattern.";
    }
};
