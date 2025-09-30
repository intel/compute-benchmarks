/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct NonUsmCopyArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    BooleanArgument updateOnHost;
    BooleanArgument reallocate;
    BooleanArgument prefetch;

    NonUsmCopyArguments()
        : sourcePlacement(*this, "src", "Placement of the source buffer"),
          destinationPlacement(*this, "dst", "Placement of the destination buffer"),
          size(*this, "size", "Size of the buffer"),
          updateOnHost(*this, "updateOnHost", "memory is updated on the host between copies"),
          reallocate(*this, "reallocate", "reallocate buffers before each copy"),
          prefetch(*this, "prefetch", "prefetch shared system buffer before each copy") {}
};

struct NonUsmCopy : TestCase<NonUsmCopyArguments> {
    using TestCase<NonUsmCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "NonUsmCopy";
    }

    std::string getHelp() const override {
        return "Measures time for non usm transfers and further potential host updates, updates/reallocates memory on the host between copies, uses immediate command lists and copy offload";
    }
};
