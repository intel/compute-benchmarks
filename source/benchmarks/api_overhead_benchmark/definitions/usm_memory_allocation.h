/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmMemoryAllocationArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument usmMemoryPlacement;
    ByteSizeArgument size;

    UsmMemoryAllocationArguments()
        : usmMemoryPlacement(*this, "type", "Type of memory being allocated"),
          size(*this, "size", "Size to allocate") {}
};

struct UsmMemoryAllocation : TestCase<UsmMemoryAllocationArguments> {
    using TestCase<UsmMemoryAllocationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmMemoryAllocation";
    }

    std::string getHelp() const override {
        return "measures time spent in USM memory allocation APIs.";
    }
};
