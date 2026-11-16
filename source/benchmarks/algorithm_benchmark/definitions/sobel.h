/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include <cstdint>

struct SobelArguments : TestCaseArgumentContainer {};

struct Sobel : TestCase<SobelArguments> {
    using TestCase<SobelArguments>::TestCase;

    static constexpr uint32_t width = 512u;
    static constexpr uint32_t height = 512u;
    static constexpr uint32_t groupSize = 16u;
    static constexpr uint32_t kernelLaunchCount = 200u;

    std::string getTestCaseName() const override {
        return "Sobel";
    }

    std::string getHelp() const override {
        return "measures the time of one workload execution: copies a generated 512x512 image to the device, launches a Sobel edge detection kernel with 16x16 work groups 200 times and copies the result to the host";
    }
};
