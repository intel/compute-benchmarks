/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_argument_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiArgumentKernelTime> registerTestCase{};

class MultiArgumentKernelTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool, size_t, bool, size_t, size_t, bool, bool>> {
};

TEST_P(MultiArgumentKernelTest, Test) {
    MultiArgumentKernelTimeArguments args{};
    args.api = std::get<0>(GetParam());
    args.argumentCount = std::get<1>(GetParam());
    args.measureSetKernelArg = std::get<2>(GetParam());
    args.useGlobalIds = std::get<3>(GetParam());
    args.count = std::get<4>(GetParam());
    args.exec = std::get<5>(GetParam());
    args.lws = std::get<6>(GetParam());
    args.groupCount = std::get<7>(GetParam());
    args.reverseOrder = std::get<8>(GetParam());
    args.useL0NewArgApi = std::get<9>(GetParam());

    MultiArgumentKernelTime test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiArgumentKernelTest,
    MultiArgumentKernelTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 4, 8, 16, 32, 64),
        ::testing::Bool(),
        ::testing::Bool(),
        ::testing::Values(1),
        ::testing::Values(false),
        ::testing::Values(32),
        ::testing::Values(8),
        ::testing::Bool(),
        ::testing::Bool()));
