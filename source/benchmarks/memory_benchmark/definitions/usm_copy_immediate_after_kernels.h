/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmCopyImmediateAfterKernelsArguments : TestCaseArgumentContainer {
    NonNegativeIntegerArgument numKernels;
    PositiveIntegerArgument numCopies;
    ByteSizeArgument size;
    PositiveIntegerArgument kernelTime;
    UsmMemoryPlacementArgument sourcePlacement;
    UsmMemoryPlacementArgument destinationPlacement;

    UsmCopyImmediateAfterKernelsArguments()
        : numKernels(*this, "numKernels", "Number of eat_time kernels appended before the copies"),
          numCopies(*this, "numCopies", "Number of zeCommandListAppendMemoryCopy calls appended after the kernels"),
          size(*this, "size", "Size of the copied buffer"),
          kernelTime(*this, "kernelTime", "Approximately how long a single kernel executes, in us"),
          sourcePlacement(*this, "src", "Placement of the source buffer"),
          destinationPlacement(*this, "dst", "Placement of the destination buffer") {}
};

struct UsmCopyImmediateAfterKernels : TestCase<UsmCopyImmediateAfterKernelsArguments> {
    using TestCase<UsmCopyImmediateAfterKernelsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyImmediateAfterKernels";
    }

    std::string getHelp() const override {
        return "Measures CPU time of appending memory copies to an in-order immediate command list "
               "that already holds time-consuming kernels. 'Append' covers the copy appends "
               "only, including any wait for the preceding kernels to complete. 'Total' "
               "covers the time from the first kernel append until synchronization returns.";
    }
};
