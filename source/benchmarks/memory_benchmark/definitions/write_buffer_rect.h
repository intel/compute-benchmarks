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

struct WriteBufferRectArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    CompressionBooleanArgument compressed;
    ThreeComponentOffsetArgument origin;
    ThreeComponentSizeArgument region;
    ByteSizeArgument rPitch;
    ByteSizeArgument sPitch;

    WriteBufferRectArguments()
        : size(*this, "size", "Size of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          origin(*this, "origin", "Origin of the rectangle"),
          region(*this, "region", "Size of the rectangle"),
          rPitch(*this, "rPitch", "Row pitch of the rectangle"),
          sPitch(*this, "sPitch", "Silice pitch of the rectangle") {}
};

struct WriteBufferRect : TestCase<WriteBufferRectArguments> {
    using TestCase<WriteBufferRectArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WriteBufferRect";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures rectangle write bandwidth. Rectangle "
               "write operation means transfer from CPU to GPU.";
    }
};
