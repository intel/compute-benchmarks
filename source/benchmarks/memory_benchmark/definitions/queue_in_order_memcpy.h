/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct QueueInOrderMemcpyArguments : TestCaseArgumentContainer {
    BooleanArgument isCopyOnly;
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    PositiveIntegerArgument count;
    BooleanArgument withCopyOffload;

    QueueInOrderMemcpyArguments()
        : isCopyOnly(*this, "IsCopyOnly", "If true, Copy Engine is selected. If false, Compute Engine is selected"),
          sourcePlacement(*this, "sourcePlacement", "Placement of the source buffer"),
          destinationPlacement(*this, "destinationPlacement", "Placement of the destination buffer"),
          size(*this, "size", "Size of memory allocation"),
          count(*this, "count", "Number of memcpy operations"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}
};

struct QueueInOrderMemcpy : TestCase<QueueInOrderMemcpyArguments> {
    using TestCase<QueueInOrderMemcpyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "QueueInOrderMemcpy";
    }

    std::string getHelp() const override {
        return "measures time on CPU spent for multiple in order memcpy.";
    }
};
