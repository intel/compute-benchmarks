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

struct ReadBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BufferContentsArgument contents;
    CompressionBooleanArgument compressed;
    BooleanArgument useEvents;
    HostptrBufferReuseModeArgument reuse;

    ReadBufferArguments()
        : size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          reuse(*this, "reuse", CommonHelpMessage::hostptrBufferReuse()) {}
};

struct ReadBuffer : TestCase<ReadBufferArguments> {
    using TestCase<ReadBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ReadBuffer";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures read bandwidth. Read operation means "
               "transfer from GPU to CPU.";
    }
};
