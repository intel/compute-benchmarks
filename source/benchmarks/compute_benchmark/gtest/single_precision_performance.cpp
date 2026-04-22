/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/single_precision_performance.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SinglePrecisionPerformance> registerTestCase{};

class SinglePrecisionPerformanceTest : public ::testing::TestWithParam<std::tuple<Api, bool>> {
};

TEST_P(SinglePrecisionPerformanceTest, Test) {
    SinglePrecisionPerformanceArguments args{};
    args.api = std::get<0>(GetParam());
    args.useEvents = std::get<1>(GetParam());

    SinglePrecisionPerformance test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SinglePrecisionPerformanceTest,
    SinglePrecisionPerformanceTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0, Api::OPT),
        ::testing::Values(true)));
