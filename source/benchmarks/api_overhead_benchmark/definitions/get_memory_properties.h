/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct GetMemoryPropertiesArguments : TestCaseArgumentContainer {
    IntegerArgument AllocationsCount;

    GetMemoryPropertiesArguments()
        : AllocationsCount(*this, "AmountOfUsmAllocations", "Amount of USM allocations that are present in system") {}
};

struct GetMemoryProperties : TestCase<GetMemoryPropertiesArguments> {
    using TestCase<GetMemoryPropertiesArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "GetMemoryProperties";
    }

    std::string getHelp() const override {
        return "measures time spent in zeMemGetAllocProperties on CPU when driver is queried for memory properties.";
    }
};
