/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include <cstdint>

struct SimpleAddArguments : TestCaseArgumentContainer {};

struct SimpleAdd : TestCase<SimpleAddArguments> {
    using TestCase<SimpleAddArguments>::TestCase;

    static constexpr uint32_t elementCount = 100000u;

    std::string getTestCaseName() const override {
        return "SimpleAdd";
    }

    std::string getHelp() const override {
        return "measures the time of one workload execution: copies 100000 integers to a device buffer, runs a single work item kernel that adds 1 to each of them in a loop and copies the result back to the host";
    }
};
