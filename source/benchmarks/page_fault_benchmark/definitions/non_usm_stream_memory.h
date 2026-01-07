/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct NonUsmStreamMemoryArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument useEvents;
    BufferContentsArgument contents;
    UsmMemoryPlacementArgument memoryPlacement;
    PositiveIntegerArgument partialMultiplier;
    PositiveIntegerArgument vectorSize;
    PositiveIntegerArgument lws;
    BooleanArgument prefetch;

    NonUsmStreamMemoryArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          contents(*this, "contents", "Buffer contents zeros/random"),
          memoryPlacement(*this, "memoryPlacement", "Memory type used for stream"),
          partialMultiplier(*this, "multiplier", "multiplies id used for accessing the resources to simulate partials"),
          vectorSize(*this, "vectorSize", "size of uint vector type 1/2/4/8/16"),
          lws(*this, "lws", "local work size"),
          prefetch(*this, "prefetch", "prefetch shared system buffer before each copy") {}
};

struct NonUsmStreamMemory : TestCase<NonUsmStreamMemoryArguments> {
    using TestCase<NonUsmStreamMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "NonUsmStreamMemory";
    }

    std::string getHelp() const override {
        return "Streams memory inside of kernel in a fashion described by 'type'. Copy means one "
               "memory location is read from and the second one is written to. Triad means two "
               "buffers are read and one is written to. In read and write memory is only read or "
               "written to.";
    }
};
