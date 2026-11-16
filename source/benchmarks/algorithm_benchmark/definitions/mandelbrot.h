/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include <cstdint>

struct MandelbrotArguments : TestCaseArgumentContainer {};

struct Mandelbrot : TestCase<MandelbrotArguments> {
    using TestCase<MandelbrotArguments>::TestCase;

    static constexpr uint32_t width = 1024u;
    static constexpr uint32_t height = 1024u;
    static constexpr uint32_t groupSize = 16u;
    static constexpr uint32_t kernelLaunchCount = 50u;

    std::string getTestCaseName() const override {
        return "Mandelbrot";
    }

    std::string getHelp() const override {
        return "measures the time of one workload execution: launches a kernel that renders a 1024x1024 Mandelbrot fractal with 16x16 work groups 50 times and copies the image to the host";
    }
};
