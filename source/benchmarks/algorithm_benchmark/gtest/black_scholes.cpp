/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/black_scholes.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BlackScholes> registerTestCase{};

class BlackScholesTest : public ::testing::TestWithParam<std::tuple<Api, DataType>> {
};

TEST_P(BlackScholesTest, Test) {
    BlackScholesArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());

    BlackScholes test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BlackScholesTest,
    BlackScholesTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Double)));
