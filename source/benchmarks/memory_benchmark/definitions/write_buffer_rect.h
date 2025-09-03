/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/hostptr_reuse_mode_argument.h"
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
    BooleanArgument inOrderQueue;
    HostptrBufferReuseModeArgument reuse;
    BufferContentsArgument contents;

    WriteBufferRectArguments()
        : size(*this, "size", "Size of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          origin(*this, "origin", "Origin of the rectangle"),
          region(*this, "region", "Size of the rectangle"),
          rPitch(*this, "rPitch", "Row pitch of the rectangle"),
          sPitch(*this, "sPitch", "Silice pitch of the rectangle"),
          inOrderQueue(*this, "inOrderQueue", "If set use IOQ, otherwise OOQ. Applicable only for OCL."),
          reuse(*this, "reuse", CommonHelpMessage::hostptrBufferReuse()),
          contents(*this, "contents", "Contents of the buffer") {}
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
