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

struct StreamAfterTransferArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    BooleanArgument useEvents;

    StreamAfterTransferArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Must be divisible by datatype size."),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct StreamAfterTransfer : TestCase<StreamAfterTransferArguments> {
    using TestCase<StreamAfterTransferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "StreamAfterTransfer";
    }

    std::string getHelp() const override {
        return "Goal of this test is to measure how stream kernels perform right after host to device transfer populating the data. "
               "Test does clean caches, then emits transfers and then follows with stream kernel and measures GPU execution time of it.";
    }
};
