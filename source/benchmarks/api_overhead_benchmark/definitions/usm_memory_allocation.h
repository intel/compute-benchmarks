/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/allocation_measure_mode_argument.h"
#include "framework/argument/enum/memory_strategy_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmMemoryAllocationArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument usmMemoryPlacement;
    ByteSizeArgument size;
    AllocationMeasureModeArgument measureMode;
    MemoryStrategyArgument strategy;

    UsmMemoryAllocationArguments()
        : usmMemoryPlacement(*this, "type", "Type of memory being allocated"),
          size(*this, "size", "Size to allocate"),
          measureMode(*this, "measureMode", "Specifies which APIs to measure"),
          strategy(*this, "strategy", "Strategy used to allocate memory (Static is not applicable here, Async is supported only by UR and SYCL)") {}
};

struct UsmMemoryAllocation : TestCase<UsmMemoryAllocationArguments> {
    using TestCase<UsmMemoryAllocationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmMemoryAllocation";
    }

    std::string getHelp() const override {
        return "measures time spent in USM memory allocation APIs using the specified strategy.";
    }
};
