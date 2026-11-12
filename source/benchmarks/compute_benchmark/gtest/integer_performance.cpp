/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/integer_performance.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<IntegerPerformance> registerTestCase{};

class IntegerPerformanceTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t>> {
};

TEST_P(IntegerPerformanceTest, Test) {
    IntegerPerformanceArguments args{};
    args.api = std::get<0>(GetParam());
    args.useEvents = std::get<1>(GetParam());
    args.vectorWidth = std::get<2>(GetParam());

    IntegerPerformance test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    IntegerPerformanceTest,
    IntegerPerformanceTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(true),
        ::testing::Values(1, 2, 4, 8, 16)));
