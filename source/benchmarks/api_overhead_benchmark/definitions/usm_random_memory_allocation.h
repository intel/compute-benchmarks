/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/allocation_measure_mode_argument.h"
#include "framework/argument/enum/distribution_kind_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmRandomMemoryAllocationArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument usmMemoryPlacement;
    PositiveIntegerArgument operationCount;
    ByteSizeArgument minSize;
    ByteSizeArgument maxSize;
    DistributionKindArgument sizeDistribution;

    UsmRandomMemoryAllocationArguments()
        : usmMemoryPlacement(*this, "type", "Type of memory being allocated"),
          operationCount(*this, "operationCount", "Total number of alloc and free operations"),
          minSize(*this, "minSize", "Minimum size per single allocation"),
          maxSize(*this, "maxSize", "Maximum size per single allocation"),
          sizeDistribution(*this, "sizeDistribution", "Specifies randomization method for allocation size") {}
};

struct UsmRandomMemoryAllocation : TestCase<UsmRandomMemoryAllocationArguments> {
    using TestCase<UsmRandomMemoryAllocationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmRandomMemoryAllocation";
    }

    std::string getHelp() const override {
        return "measures time spent in USM memory allocation APIs, with a randomized scenario. ";
    }
};
