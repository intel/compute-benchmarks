/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_launch_kernel.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendLaunchKernel> registerTestCase{};

class AppendLaunchKernelTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, uint32_t>> {
};

TEST_P(AppendLaunchKernelTest, Test) {
    AppendLaunchKernelArguments args{};
    args.api = Api::L0;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useEvent = std::get<2>(GetParam());
    args.appendCount = std::get<3>(GetParam());

    AppendLaunchKernel test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendLaunchKernelTest,
    AppendLaunchKernelTest,
    ::testing::Combine(
        ::testing::Values(1, 1000),
        ::testing::Values(1, 256),
        ::testing::Values(false, true),
        ::testing::Values(100)));
