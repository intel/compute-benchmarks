/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/test_case/test_case.h"

struct StreamMemoryImmediateArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument useEvents;

    StreamMemoryImmediateArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct StreamMemoryImmediate : TestCase<StreamMemoryImmediateArguments> {
    using TestCase<StreamMemoryImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "StreamMemoryImmediate";
    }

    std::string getHelp() const override {
        return "Streams memory inside of kernel in a fashion described by 'type' using immediate command list. "
               "Copy means one memory location is read from and the second one is written to. Triad means two "
               "buffers are read and one is written to. In read and write memory is only read or "
               "written to.";
    }
};
