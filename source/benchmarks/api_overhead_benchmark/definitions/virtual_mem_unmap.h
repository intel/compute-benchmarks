/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemUnMapArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument reserveSize;

    VirtualMemUnMapArguments()
        : reserveSize(*this, "reserveSize", "Size in bytes to be unmapped") {}
};

struct VirtualMemUnMap : TestCase<VirtualMemUnMapArguments> {
    using TestCase<VirtualMemUnMapArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemUnMap";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemUnMap on CPU.";
    }
};
