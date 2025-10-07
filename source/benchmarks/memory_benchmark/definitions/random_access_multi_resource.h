/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct RandomAccessMultiResourceArguments : TestCaseArgumentContainer {
    ByteSizeArgument firstSize;
    ByteSizeArgument secondSize;
    UsmMemoryPlacementArgument firstPlacement;
    UsmMemoryPlacementArgument secondPlacement;

    RandomAccessMultiResourceArguments()
        : firstSize(*this, "firstSize", "Size of first resource. (Maximum supported is 16GB)"),
          secondSize(*this, "secondSize", "Size of second resource. (Maximum supported is 16GB)"),
          firstPlacement(*this, "firstPlacement", "Placement of first resource"),
          secondPlacement(*this, "secondPlacement", "Placement of second resource") {}
};

struct RandomAccessMultiResource : TestCase<RandomAccessMultiResourceArguments> {
    using TestCase<RandomAccessMultiResourceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RandomAccessMultiResource";
    }

    std::string getHelp() const override {
        return "Measures random access bandwidth for different allocation sizes and placements."
               "The benchmark uses 10 million accesses to memory for each resource.";
    }
};
