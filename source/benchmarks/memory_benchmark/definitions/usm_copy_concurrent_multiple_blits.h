/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

constexpr uint32_t maxNumberOfEngines = 9;

using BcsBitmaskArgument = BitmaskArgument<maxNumberOfEngines, false>;

struct UsmCopyConcurrentMultipleBlitsArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BcsBitmaskArgument h2dBlitters;
    BcsBitmaskArgument d2hBlitters;

    UsmCopyConcurrentMultipleBlitsArguments()
        : size(*this, "size", "Size of the copy to be done for each copy engine"),
          h2dBlitters(*this, "h2dBlitters", "A bit mask for selecting copy engines to be used for host to device copy"),
          d2hBlitters(*this, "d2hBlitters", "A bit mask for selecting copy engines to be used for device to host copy") {}
};

struct UsmCopyConcurrentMultipleBlits : TestCase<UsmCopyConcurrentMultipleBlitsArguments> {
    using TestCase<UsmCopyConcurrentMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyConcurrentMultipleBlits";
    }

    std::string getHelp() const override {
        return "Measures Copy bandwidth while performing concurrent copies between host and device using "
               "different copy engines. Engines for Host to Device copies could be selected using h2dBlitters. "
               "Engines for Device to Host copies could be selected using d2hBlitters.";
    }
};
