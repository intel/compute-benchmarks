/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/map_flags_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UnmapBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BufferContentsArgument contents;
    CompressionBooleanArgument compressed;
    MapFlagsArgument mapFlags;
    BooleanArgument useEvents;

    UnmapBufferArguments()
        : size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          mapFlags(*this, "mapFlags", "OpenCL map flags passed during memory mapping"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct UnmapBuffer : TestCase<UnmapBufferArguments> {
    using TestCase<UnmapBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UnmapBuffer";
    }

    std::string getHelp() const override {
        return "allocates an OpenCL buffer and measures unmap bandwidth. Unmapping operation means"
               "memory transfer from CPU to GPU or a no-op, depending on map flags.";
    }
};
