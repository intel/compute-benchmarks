/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

constexpr uint32_t maxNumberOfEngines = 9;

using BcsBitmaskArgument = BitmaskArgument<maxNumberOfEngines, false>;

struct UsmP2PCopyMultipleBlitsArguments : TestCaseArgumentContainer {
    IntegerArgument srcDeviceId;
    IntegerArgument dstDeviceId;
    ByteSizeArgument size;
    BcsBitmaskArgument blitters;

    UsmP2PCopyMultipleBlitsArguments()
        : srcDeviceId(*this, "srcDeviceId", "Source device"),
          dstDeviceId(*this, "dstDeviceId", "Destination device"),
          size(*this, "size", "Size of the operation processed by each engine"),
          blitters(*this, "blitters", "A bit mask for selecting copy engines") {}
};

struct UsmCopyMultipleBlits : TestCase<UsmP2PCopyMultipleBlitsArguments> {
    using TestCase<UsmP2PCopyMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyMultipleBlits";
    }

    std::string getHelp() const override {
        return "allocates two unified device memory buffers on separate devices and performs a copy "
               "between sections (or chunks) of these using a different copy engine and measures bandwidth. "
               "Test first checks for P2P capabilities in the target platform before submitting the copy. "
               "Results for each individual blitter engine is measured using GPU-based timings and reported "
               "separately. Total bandwidths are calculated by dividing the total buffer size by "
               "the worst result from all engines. Division of work among blitters is not always "
               "even - if main copy engine is specified (rightmost bit in --bliters argument), it "
               "gets a half of the buffer and the rest is divided between remaining copy engines. "
               "Otherwise the division is even.";
    }
};
