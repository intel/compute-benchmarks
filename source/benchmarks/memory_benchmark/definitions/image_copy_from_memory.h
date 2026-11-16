/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/image_format_layout_argument.h"
#include "framework/argument/enum/image_format_type_argument.h"
#include "framework/argument/enum/image_type_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct ImageCopyFromMemoryArguments : TestCaseArgumentContainer {
    ImageTypeArgument imageType;
    ThreeComponentSizeArgument size;
    ImageFormatLayoutArgument layout;
    ImageFormatTypeArgument formatType;
    UsmMemoryPlacementArgument sourcePlacement;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    ImageCopyFromMemoryArguments()
        : imageType(*this, "imageType", "Type of the image"),
          size(*this, "size", "Size of the image in pixels as width:height:depth. Array layers are the height of a 1D array and the depth of a 2D array"),
          layout(*this, "layout", "Format layout of the image. NV12 and P010 need a 2D image with even width and height"),
          formatType(*this, "formatType", "Format type of the image"),
          sourcePlacement(*this, "src", "Placement of the source memory"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct ImageCopyFromMemory : TestCase<ImageCopyFromMemoryArguments> {
    using TestCase<ImageCopyFromMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ImageCopyFromMemory";
    }

    std::string getHelp() const override {
        return "allocates an image and a memory buffer and measures the bandwidth of copying the buffer into the image with zeCommandListAppendImageCopyFromMemory on an in-order immediate command list. Planar layouts are copied per plane through planar image views.";
    }
};
