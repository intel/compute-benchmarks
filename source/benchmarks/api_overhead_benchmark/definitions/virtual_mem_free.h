/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemFreeArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument freeSize;

    VirtualMemFreeArguments()
        : freeSize(*this, "freeSize", "Size in bytes to be freed") {}
};

struct VirtualMemFree : TestCase<VirtualMemFreeArguments> {
    using TestCase<VirtualMemFreeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemFree";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemFree on CPU.";
    }
};
