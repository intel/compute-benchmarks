/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"

struct ImageRecreationArguments : TestCaseArgumentContainer {
    ThreeComponentSizeArgument size;
    BooleanArgument writeWholeImage;

    ImageRecreationArguments()
        : size(*this, "size", "Size of the image"),
          writeWholeImage(*this, "writeWholeImage", "Write the whole image on the GPU, as opposed to a single pixel") {}
};

struct ImageRecreation : TestCase<ImageRecreationArguments> {
    using TestCase<ImageRecreationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ImageRecreation";
    }

    std::string getHelp() const override {
        return "measures time used to create a temporary image, write to it on the GPU, wait for the write and release the image, "
               "as in a frame loop that allocates a new image for every frame.";
    }
};
