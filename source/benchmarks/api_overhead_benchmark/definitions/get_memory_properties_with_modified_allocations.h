/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct GetMemoryPropertiesWithModifiedAllocationsArguments : TestCaseArgumentContainer {
    IntegerArgument AllocationsCount;

    GetMemoryPropertiesWithModifiedAllocationsArguments()
        : AllocationsCount(*this, "AmountOfUsmAllocations", "Amount of USM allocations that are present in system") {}
};

struct GetMemoryPropertiesWithModifiedAllocations : TestCase<GetMemoryPropertiesWithModifiedAllocationsArguments> {
    using TestCase<GetMemoryPropertiesWithModifiedAllocationsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "GetMemoryPropertiesWithModifiedAllocations";
    }

    std::string getHelp() const override {
        return "measures time spent in zeMemGetAllocProperties on CPU, when allocations are modified between each iteration.";
    }
};
