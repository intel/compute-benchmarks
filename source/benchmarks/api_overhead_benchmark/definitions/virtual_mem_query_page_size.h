/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemQueryPageSizeArguments : TestCaseArgumentContainer {
    VirtualMemQueryPageSizeArguments() {}
};

struct VirtualMemQueryPageSize : TestCase<VirtualMemQueryPageSizeArguments> {
    using TestCase<VirtualMemQueryPageSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VirtualMemQueryPageSize";
    }

    std::string getHelp() const override {
        return "measures time spent in zeVirtualMemQueryPageSize on CPU.";
    }
};
