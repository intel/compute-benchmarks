/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct KernelDurationArguments : TestCaseArgumentContainer {};

struct KernelDuration : TestCase<KernelDurationArguments> {
    using TestCase<KernelDurationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelDuration";
    }

    std::string getHelp() const override {
        return "measures GPU execution time (kernel start to kernel end) of a kernel in which every work item "
               "sums 16 floats from device memory. Work group size is the maximum group size and the work item "
               "count is the maximum group size times the number of EUs.";
    }
};
