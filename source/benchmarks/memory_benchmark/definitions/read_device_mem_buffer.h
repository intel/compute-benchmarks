/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct ReadDeviceMemBufferArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    CompressionBooleanArgument compressed;
    // ByteSizeArgument numWaves;

    ReadDeviceMemBufferArguments()
        : size(*this, "size", "Size of the buffer"),
          compressed(*this, "compressed", CommonHelpMessage::compression("buffer"))
    //,numWaves(*this, "waves")
    {}
};

struct ReadDeviceMemBuffer : TestCase<ReadDeviceMemBufferArguments> {
    using TestCase<ReadDeviceMemBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ReadDeviceMemBuffer";
    }

    std::string getHelp() const override {
        return "allocates two OpenCL buffers and measures source buffer read bandwidth. Source "
               "buffer resides in device memory.";
    }
};
