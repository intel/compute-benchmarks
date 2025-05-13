/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/int64_div.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Int64Div> registerTestCase{};

class Int64DivTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool>> {
};

TEST_P(Int64DivTest, Test) {
    Int64DivArguments args{};
    args.api = Api::OpenCL;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useEvents = std::get<2>(GetParam());

    Int64Div test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    Int64DivTest,
    Int64DivTest,
    ::testing::Combine(
        ::testing::Values(1, 128),
        ::testing::Values(64, 128, 256, 512),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    Int64DivTestLIMITED,
    Int64DivTest,
    ::testing::Combine(
        ::testing::Values(128),
        ::testing::Values(512),
        ::testing::Values(true)));
