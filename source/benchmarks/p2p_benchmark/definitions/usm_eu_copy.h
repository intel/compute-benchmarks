/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmP2PCopyArguments : TestCaseArgumentContainer {
    IntegerArgument srcDeviceId;
    IntegerArgument dstDeviceId;
    ByteSizeArgument size;
    BufferContentsArgument contents;
    BooleanArgument useEvents;
    BooleanArgument reuseCommandList;

    UsmP2PCopyArguments()
        : srcDeviceId(*this, "srcDeviceId", "Source device"),
          dstDeviceId(*this, "dstDeviceId", "Destination device"),
          size(*this, "size", "Size of the buffer"),
          contents(*this, "contents", "Contents of the buffers"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          reuseCommandList(*this, "reuseCmdList", "Command list is reused between iterations") {}
};

struct UsmEUCopy : TestCase<UsmP2PCopyArguments> {
    using TestCase<UsmP2PCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEUCopy";
    }

    std::string getHelp() const override {
        return "allocates two unified device memory buffers on separate devices, performs a copy "
               "between them using a compute engine, and reports bandwidth. Test first checks for "
               "P2P capabilities in the target platform before submitting the copy.";
    }
};
