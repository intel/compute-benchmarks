/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemMapArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument reserveSize;
    BooleanArgument useOffset;
    StringArgument accessType;

    VirtualMemMapArguments()
        : reserveSize(*this, "reserveSize", "Size in bytes to be reserved"),
          useOffset(*this, "useOffset", "Use offset to map into physical memory"),
          accessType(*this, "accessType", "Access type. Either 'ReadWrite' or 'ReadOnly'") {}
};

struct VirtualMemMap : TestCase<VirtualMemMapArguments> {
    using TestCase<VirtualMemMapArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemMap";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemMap on CPU.";
    }
};
