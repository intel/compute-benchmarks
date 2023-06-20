/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct GetMemoryPropertiesWithOffsetedPointerArguments : TestCaseArgumentContainer {
    IntegerArgument AllocationsCount;

    GetMemoryPropertiesWithOffsetedPointerArguments()
        : AllocationsCount(*this, "AmountOfUsmAllocations", "Amount of USM allocations that are present in system") {}
};

struct GetMemoryPropertiesWithOffsetedPointer : TestCase<GetMemoryPropertiesWithOffsetedPointerArguments> {
    using TestCase<GetMemoryPropertiesWithOffsetedPointerArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "GetMemoryPropertiesWithOffsetedPointer";
    }

    std::string getHelp() const override {
        return "measures time spent in zeMemGetAllocProperties on CPU when the pointer passed is an offset from the base address.";
    }
};
