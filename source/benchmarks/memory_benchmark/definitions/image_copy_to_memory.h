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

struct ImageCopyToMemoryArguments : TestCaseArgumentContainer {
    ImageTypeArgument imageType;
    ThreeComponentSizeArgument size;
    ImageFormatLayoutArgument layout;
    ImageFormatTypeArgument formatType;
    UsmMemoryPlacementArgument destinationPlacement;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    ImageCopyToMemoryArguments()
        : imageType(*this, "imageType", "Type of the image"),
          size(*this, "size", "Size of the image in pixels as width:height:depth. Array layers are the height of a 1D array and the depth of a 2D array"),
          layout(*this, "layout", "Format layout of the image. NV12 and P010 need a 2D image with even width and height"),
          formatType(*this, "formatType", "Format type of the image"),
          destinationPlacement(*this, "dst", "Placement of the destination memory"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct ImageCopyToMemory : TestCase<ImageCopyToMemoryArguments> {
    using TestCase<ImageCopyToMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ImageCopyToMemory";
    }

    std::string getHelp() const override {
        return "allocates an image and a memory buffer and measures the bandwidth of copying the image into the buffer with zeCommandListAppendImageCopyToMemory on an in-order immediate command list. Planar layouts are copied per plane through planar image views.";
    }
};
