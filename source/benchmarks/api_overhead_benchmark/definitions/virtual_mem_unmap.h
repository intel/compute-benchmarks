/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct VirtualMemUnMapArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument reserveSize;
    PositiveIntegerArgument poolSize;

    VirtualMemUnMapArguments()
        : reserveSize(*this, "reserveSize", "Size in bytes to be unmapped"),
          poolSize(*this, "poolSize", "Number of virtual/physical memory reservations to pool and cycle through") {
        poolSize = 1;
    }
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
