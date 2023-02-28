/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct PhysicalMemCreateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument reserveSize;

    PhysicalMemCreateArguments()
        : reserveSize(*this, "reserveSize", "Size in bytes to be reserved") {}
};

struct PhysicalMemCreate : TestCase<PhysicalMemCreateArguments> {
    using TestCase<PhysicalMemCreateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "PhysicalMemCreate";
    }

    std::string getHelp() const override {
        return "measures time spent in zePhysicalMemCreate on CPU.";
    }
};
