/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/device_selection_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct CopyBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BufferContentsArgument contents;
    CompressionBooleanArgument compressedSource;
    CompressionBooleanArgument compressedDestination;
    BooleanArgument useEvents;

    CopyBufferArguments()
        : size(*this, "size", "Size of the buffers"),
          contents(*this, "contents", "Contents of the buffers"),
          compressedSource(*this, "compressedSource", CommonHelpMessage::compression("source buffer")),
          compressedDestination(*this, "compressedDestination", CommonHelpMessage::compression("destination buffer")),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct CopyBuffer : TestCase<CopyBufferArguments> {
    using TestCase<CopyBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyBuffer";
    }

    std::string getHelp() const override {
        return "allocates two OpenCL buffers and measures copy bandwidth between them. Buffers "
               "will be placed in device memory, if it's available.";
    }
};
