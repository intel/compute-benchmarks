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

struct ReadBufferRectArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    CompressionBooleanArgument compressed;
    ThreeComponentOffsetArgument origin;
    ThreeComponentSizeArgument region;
    ByteSizeArgument rPitch;
    ByteSizeArgument sPitch;

    ReadBufferRectArguments()
        : size(*this, "size", "Size of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          origin(*this, "origin", "Origin of the rectangle"),
          region(*this, "region", "Size of the rectangle"),
          rPitch(*this, "rPitch", "Row pitch of the rectangle"),
          sPitch(*this, "sPitch", "Silice pitch of the rectangle") {}
};

struct ReadBufferRect : TestCase<ReadBufferRectArguments> {
    using TestCase<ReadBufferRectArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ReadBufferRect";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures rectangle read bandwidth. Rectangle "
               "read operation means transfer from GPU to CPU.";
    }
};
