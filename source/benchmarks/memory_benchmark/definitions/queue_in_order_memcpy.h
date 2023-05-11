/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct QueueInOrderMemcpyArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;
    PositiveIntegerArgument count;

    QueueInOrderMemcpyArguments()
        : sourcePlacement(*this, "sourcePlacement", "Placement of the source buffer"),
          destinationPlacement(*this, "destinationPlacement", "Placement of the destination buffer"),
          size(*this, "size", "Size of memory allocation"),
          count(*this, "count", "Number of memcpy operations") {}
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
