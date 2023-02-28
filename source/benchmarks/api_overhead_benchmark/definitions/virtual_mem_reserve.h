/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemReserveArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument reserveSize;
    BooleanArgument useNull;

    VirtualMemReserveArguments()
        : reserveSize(*this, "reserveSize", "Size in bytes to be reserved"),
          useNull(*this, "useNull", "Flag to decide whether Null to be used for start of region") {}
};

struct VirtualMemReserve : TestCase<VirtualMemReserveArguments> {
    using TestCase<VirtualMemReserveArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemReserve";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemReserve on CPU.";
    }
};
