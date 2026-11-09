/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct UsmCopyReservedMemoryArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    PositiveIntegerArgument mapCount;
    ByteSizeArgument chunkSize;
    BooleanArgument measureCompletionTime;

    UsmCopyReservedMemoryArguments()
        : size(*this, "size", "Size of memory to copy. Source and destination are placed back to back at the start of the reservation"),
          mapCount(*this, "mapCount", "Number of zeVirtualMemMap calls, each mapping its own physical memory chunk into the reservation"),
          chunkSize(*this, "chunkSize", "Size of each mapped chunk, rounded up to the virtual memory page size"),
          measureCompletionTime(*this, "measureCompletionTime", "Measures time taken to complete the copy (default is to measure only the append call)") {}
};

struct UsmCopyReservedMemory : TestCase<UsmCopyReservedMemoryArguments> {
    using TestCase<UsmCopyReservedMemoryArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyReservedMemory";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendMemoryCopy on CPU on an immediate command list, when the source and destination "
               "are inside a virtual memory reservation that is mapped with the given number of zeVirtualMemMap calls.";
    }
};
