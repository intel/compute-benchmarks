/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemGetAccessAttribArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument size;
    VirtualMemGetAccessAttribArguments()
        : size(*this, "size", "Size in bytes to get the access attribute") {}
};

struct VirtualMemGetAccessAttrib : TestCase<VirtualMemGetAccessAttribArguments> {
    using TestCase<VirtualMemGetAccessAttribArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemGetAccessAttrib";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemGetAccessAttribute on CPU.";
    }
};
