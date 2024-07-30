/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_argument_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiArgumentKernelTime> registerTestCase{};

class MultiArgumentKernelTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(MultiArgumentKernelTest, Test) {
    MultiArgumentKernelTimeArguments args{};
    args.api = Api::OpenCL;
    args.argumentCount = std::get<0>(GetParam());
    args.measureSetKernelArg = std::get<1>(GetParam());

    MultiArgumentKernelTime test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiArgumentKernelTest,
    MultiArgumentKernelTest,
    ::testing::Combine(
        ::testing::Values(1, 4, 8, 16, 32, 64),
        ::testing::Bool()));
