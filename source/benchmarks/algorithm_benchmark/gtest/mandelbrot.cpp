/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/mandelbrot.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Mandelbrot> registerTestCase{};

class MandelbrotTest : public ::testing::TestWithParam<Api> {
};

TEST_P(MandelbrotTest, Test) {
    MandelbrotArguments args{};
    args.api = GetParam();

    Mandelbrot test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MandelbrotTest,
    MandelbrotTest,
    ::testing::Values(Api::OpenCL, Api::L0));
