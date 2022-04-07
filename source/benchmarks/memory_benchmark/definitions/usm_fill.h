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
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmFillArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument usmMemoryPlacement;
    ByteSizeArgument bufferSize;
    BufferContentsArgument contents;
    ByteSizeArgument patternSize;
    BufferContentsArgument patternContents;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    UsmFillArguments()
        : usmMemoryPlacement(*this, "memory", "Placement of the buffer"),
          bufferSize(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          patternSize(*this, "patternSize", "Size of the fill pattern"),
          patternContents(*this, "patternContents", "Select contents of the fill pattern"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct UsmFill : TestCase<UsmFillArguments> {
    using TestCase<UsmFillArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmFill";
    }

    std::string getHelp() const override {
        return "allocates a unified memory buffer and measures fill bandwidth";
    }
};
