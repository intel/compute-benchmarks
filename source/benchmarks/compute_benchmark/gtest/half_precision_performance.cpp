/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/half_precision_performance.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<HalfPrecisionPerformance> registerTestCase{};

class HalfPrecisionPerformanceTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t>> {
};

TEST_P(HalfPrecisionPerformanceTest, Test) {
    HalfPrecisionPerformanceArguments args{};
    args.api = std::get<0>(GetParam());
    args.useEvents = std::get<1>(GetParam());
    args.vectorWidth = std::get<2>(GetParam());

    HalfPrecisionPerformance test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    HalfPrecisionPerformanceTest,
    HalfPrecisionPerformanceTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(true),
        ::testing::Values(1, 2, 4, 8, 16)));
