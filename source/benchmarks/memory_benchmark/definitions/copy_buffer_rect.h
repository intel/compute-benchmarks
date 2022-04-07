/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/three_component_uint_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct CopyBufferRectArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    CompressionBooleanArgument compressedSource;
    CompressionBooleanArgument compressedDestination;
    ThreeComponentOffsetArgument origin;
    ThreeComponentSizeArgument region;
    ByteSizeArgument rPitch;
    ByteSizeArgument sPitch;

    CopyBufferRectArguments()
        : size(*this, "size", "Size of the buffer"),
          compressedSource(*this, "srcCompressed", CommonHelpMessage::compression("source buffer")),
          compressedDestination(*this, "dstCompressed", CommonHelpMessage::compression("destination buffer")),
          origin(*this, "origin", "Origin of the rectangle"),
          region(*this, "region", "Size of the rectangle"),
          rPitch(*this, "rPitch", "Row pitch of the rectangle"),
          sPitch(*this, "sPitch", "Silice pitch of the rectangle") {}
};

struct CopyBufferRect : TestCase<CopyBufferRectArguments> {
    using TestCase<CopyBufferRectArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyBufferRect";
    }

    std::string getHelp() const override {
        return "allocates two OpenCL buffers and measures rectangle copy bandwidth betweem them. "
               "Buffers will be placed in device memory, if it's available.";
    }
};
