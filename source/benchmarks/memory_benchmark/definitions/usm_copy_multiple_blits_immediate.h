/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

constexpr uint32_t maxNumberOfEngines = 9;

using BcsBitmaskArgument = BitmaskArgument<maxNumberOfEngines, false>;

struct UsmImmediateCopyCopyMultipleBlitsArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    BcsBitmaskArgument blitters;

    UsmImmediateCopyCopyMultipleBlitsArguments()
        : sourcePlacement(*this, "src", "Placement of the source buffer"),
          destinationPlacement(*this, "dst", "Placement of the destination buffer"),
          size(*this, "size", "Size of the operation processed by each engine"),
          blitters(*this, "blitters", "A bit mask for selecting copy engines") {}
};

struct UsmImmediateCopyMultipleBlits : TestCase<UsmImmediateCopyCopyMultipleBlitsArguments> {
    using TestCase<UsmImmediateCopyCopyMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmImmediateCopyMultipleBlits";
    }

    std::string getHelp() const override {
        return "allocates two unified shared memory buffers, divides them into chunks, copies "
               "each chunk using a different copy engine with an immediate command list and "
               " measures bandwidth. Results for each "
               "individual blitter engine is measured using GPU-based timings and reported "
               "separately. Total bandwidths are calculated by dividing the total buffer size by "
               "the worst result from all engines. Division of work among blitters is not always "
               "even - if main copy engine is specified (rightmost bit in --bliters argument), it "
               "gets a half of the buffer and the rest is divided between remaining copy engines. "
               "Otherwise the division is even.";
    }
};
