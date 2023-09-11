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

struct MemGetIpcHandleArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    IntegerArgument AllocationsCount;

    MemGetIpcHandleArguments()
        : sourcePlacement(*this, "src", "Placement of the source buffer"),
          AllocationsCount(*this, "AmountOfUsmAllocations", "Amount of USM allocations that are present in system") {}
};

struct MemGetIpcHandle : TestCase<MemGetIpcHandleArguments> {
    using TestCase<MemGetIpcHandleArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MemGetIpcHandle";
    }

    std::string getHelp() const override {
        return "measures time spent in zeMemGetIpcHandle on CPU.";
    }
};
