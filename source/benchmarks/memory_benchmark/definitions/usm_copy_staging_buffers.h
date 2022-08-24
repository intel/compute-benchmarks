/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmCopyStagingBuffersArguments : TestCaseArgumentContainer {
    BooleanArgument forceBlitter;
    UsmMemoryPlacementArgument dstPlacement;
    ByteSizeArgument size;
    PositiveIntegerArgument chunks;

    UsmCopyStagingBuffersArguments()
        : forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          dstPlacement(*this, "dst", "Memory placement of destination"),
          size(*this, "size", "Size of the buffer"),
          chunks(*this, "chunks", "How much memory chunks should the buffer be splitted into") {}
};

struct UsmCopyStagingBuffers : TestCase<UsmCopyStagingBuffersArguments> {
    using TestCase<UsmCopyStagingBuffersArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyStagingBuffers";
    }

    std::string getHelp() const override {
        return "Measures copy time from device/host to host/device. Host memory is non-USM allocation."
               "Copy is done through staging USM buffers. Non-USM host ptr is never passed to L0 API, only through staging buffers.";
    }
};
