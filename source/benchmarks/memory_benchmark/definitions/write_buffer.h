/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/hostptr_reuse_mode_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct WriteBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BufferContentsArgument contents;
    CompressionBooleanArgument compressed;
    BooleanArgument useEvents;
    HostptrBufferReuseModeArgument reuse;

    WriteBufferArguments()
        : size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          reuse(*this, "reuse", CommonHelpMessage::hostptrBufferReuse()) {}
};

struct WriteBuffer : TestCase<WriteBufferArguments> {
    using TestCase<WriteBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WriteBuffer";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures write bandwidth. Write operation means "
               "transfer from CPU to GPU.";
    }
};
