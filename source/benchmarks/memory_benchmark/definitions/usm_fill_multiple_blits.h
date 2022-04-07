/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

constexpr uint32_t maxNumberOfEngines = 9;

using BcsBitmaskArgument = BitmaskArgument<maxNumberOfEngines, false>;

struct UsmFillMultipleBlitsArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument memoryPlacement;
    ByteSizeArgument size;
    ByteSizeArgument patternSize;
    BufferContentsArgument patternContents;
    BcsBitmaskArgument blitters;

    UsmFillMultipleBlitsArguments()
        : memoryPlacement(*this, "memory", "Placement of buffer"),
          size(*this, "size", "Size of the operation processed by each engine"),
          patternSize(*this, "patternSize", "Size of the fill pattern"),
          patternContents(*this, "patternContents", "Select contents of the fill pattern"),
          blitters(*this, "blitters", "A bit mask for selecting copy engines") {}
};

struct UsmFillMultipleBlits : TestCase<UsmFillMultipleBlitsArguments> {
    using TestCase<UsmFillMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmFillMultipleBlits";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer, divides it into chunks, copies each "
               "chunk using a different copy engine and measures bandwidth. Refer to "
               "UsmCopyMultipleBlits for more details.";
    }
};
