/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_launch_kernel_with_parameters.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendLaunchKernelWithParameters> registerTestCase{};

class AppendLaunchKernelWithParametersTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, uint32_t, bool>> {
};

TEST_P(AppendLaunchKernelWithParametersTest, Test) {
    AppendLaunchKernelWithParametersArguments args{};
    args.api = Api::L0;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useEvent = std::get<2>(GetParam());
    args.appendCount = std::get<3>(GetParam());
    args.useCooperative = std::get<4>(GetParam());

    AppendLaunchKernelWithParameters test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendLaunchKernelWithParametersTest,
    AppendLaunchKernelWithParametersTest,
    ::testing::Combine(
        ::testing::Values(1, 10, 100),
        ::testing::Values(1, 64, 256),
        ::testing::Values(false, true),
        ::testing::Values(100),
        ::testing::Values(false, true)));
