/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/allocation_measure_mode_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmBatchMemoryAllocationArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument usmMemoryPlacement;
    PositiveIntegerArgument allocationCount;
    ByteSizeArgument size;
    AllocationMeasureModeArgument measureMode;

    UsmBatchMemoryAllocationArguments()
        : usmMemoryPlacement(*this, "type", "Type of memory being allocated"),
          allocationCount(*this, "allocationCount", "Number of allocations done"),
          size(*this, "size", "Size per single allocation"),
          measureMode(*this, "measureMode", "Specifies which APIs to measure") {}
};

struct UsmBatchMemoryAllocation : TestCase<UsmBatchMemoryAllocationArguments> {
    using TestCase<UsmBatchMemoryAllocationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmBatchMemoryAllocation";
    }

    std::string getHelp() const override {
        return "measures time spent in USM memory allocation APIs, when many allocations are requested. ";
    }
};
