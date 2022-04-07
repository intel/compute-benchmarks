/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct FillBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BufferContentsArgument contents;
    ByteSizeArgument patternSize;
    CompressionBooleanArgument compressed;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    FillBufferArguments()
        : size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          patternSize(*this, "patternSize", "Size of the fill pattern"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct FillBuffer : TestCase<FillBufferArguments> {
    using TestCase<FillBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "FillBuffer";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures fill bandwidth. Buffer will be placed in "
               "device memory, if it's available.";
    }
};
