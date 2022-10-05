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

struct QueueMemcpyArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;
    ByteSizeArgument size;

    QueueMemcpyArguments()
        : sourcePlacement(*this, "sourcePlacement", "Placement of the source buffer"),
          destinationPlacement(*this, "destinationPlacement", "Placement of the destination buffer"),
          size(*this, "size", "Size of memory allocation") {}
};

struct QueueMemcpy : TestCase<QueueMemcpyArguments> {
    using TestCase<QueueMemcpyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "QueueMemcpy";
    }

    std::string getHelp() const override {
        return "measures time spent in SYCL queue memcpy on CPU.";
    }
};
