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
#include "framework/argument/enum/usm_device_selection_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmBidirectionalCopyArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument write;
    BooleanArgument forceBlitter;

    UsmBidirectionalCopyArguments()
        : size(*this, "size", "Size of the buffers"),
          write(*this, "write", CommonHelpMessage::writeOperation()),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct UsmBidirectionalCopy : TestCase<UsmBidirectionalCopyArguments> {
    using TestCase<UsmBidirectionalCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmBidirectionalCopy";
    }

    std::string getHelp() const override {
        return "allocates two unified device memory buffers, each on a different tile, "
               "and measures copy bandwidth between. Test measures copies on two directions, "
               "which can be controlled with the -write parameter: with -write=1, each tile "
               "performs a write operation. For instance: queue is placed in tile 0, source is "
               "buffer in tile 0, and destination is in tile 1. Similarly for tile 1, queue is "
               "placed in tile 1, source in tile 1, and destination in tile 0. With -write=0, "
               "the destination and source are flipped: queue is placed in tile 0, source is "
               "buffer in tile 1, and destination is in tile 0, while for tile 1, queue is "
               "placed in tile 1, source in tile 0, and destination in tile 1.";
    }
};
