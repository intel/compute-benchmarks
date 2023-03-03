/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/buffer_contents_argument.h"
#include "framework/test_case/test_case.h"

struct RandomAccessArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument allocationSize;
    PositiveIntegerArgument alignment;
    StringArgument accessMode;
    PositiveIntegerArgument randomAccessRange;

    RandomAccessArguments()
        : allocationSize(*this, "allocationSize", "Size of device memory to be allocated.(Maximum supported is 16GB)"),
          alignment(*this, "alignment", "Alignment request for the allocated memory"),
          accessMode(*this, "accessMode", "Access mode to be used('Read', 'Write', 'ReadWrite')"),
          randomAccessRange(*this, "randomAccessRange", "Percentage of allocation size to be used for random access") {}
};

struct RandomAccess : TestCase<RandomAccessArguments> {
    using TestCase<RandomAccessArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RandomAccessMemory";
    }

    std::string getHelp() const override {
        return "Measures device-memory random access bandwidth for different allocation sizes, alignments and access modes."
               "The benchmark uses 10 million accesses to memory.";
    }
};
