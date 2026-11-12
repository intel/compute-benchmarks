/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/single_precision_vector_performance.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SinglePrecisionVectorPerformance> registerTestCase{};

class SinglePrecisionVectorPerformanceTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t>> {
};

TEST_P(SinglePrecisionVectorPerformanceTest, Test) {
    SinglePrecisionVectorPerformanceArguments args{};
    args.api = std::get<0>(GetParam());
    args.useEvents = std::get<1>(GetParam());
    args.vectorWidth = std::get<2>(GetParam());

    SinglePrecisionVectorPerformance test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SinglePrecisionVectorPerformanceTest,
    SinglePrecisionVectorPerformanceTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(true),
        ::testing::Values(2, 4, 8, 16)));
