/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemSetAccessAttribArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument size;
    StringArgument accessType;

    VirtualMemSetAccessAttribArguments()
        : size(*this, "size", "Size in bytes to set the access attribute"),
          accessType(*this, "accessType", "Access type to set. Either 'ReadWrite', 'ReadOnly' or 'None'") {}
};

struct VirtualMemSetAccessAttrib : TestCase<VirtualMemSetAccessAttribArguments> {
    using TestCase<VirtualMemSetAccessAttribArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemSetAccessAttrib";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemSetAccessAttribute on CPU.";
    }
};
