/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/device_selection_argument.h"
#include "framework/argument/enum/multi_device_selection_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct WriteBufferArguments : TestCaseArgumentContainer {
    MultiDeviceSelectionArgument contextPlacement;
    DeviceSelectionArgument queuePlacement;
    DeviceSelectionArgument bufferPlacement;
    ByteSizeArgument size;
    CompressionBooleanArgument compressed;
    BooleanArgument useEvents;

    WriteBufferArguments()
        : contextPlacement(*this, "context", "How context will be created"),
          queuePlacement(*this, "queue", "Which device within the context will perform the operation"),
          bufferPlacement(*this, "memory", "Placement of memory for the buffer"),
          size(*this, "size", "Size of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer")),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        return DeviceSelectionHelper::isSubset(contextPlacement, queuePlacement) &&
               DeviceSelectionHelper::isSubset(contextPlacement, bufferPlacement);
    }
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
