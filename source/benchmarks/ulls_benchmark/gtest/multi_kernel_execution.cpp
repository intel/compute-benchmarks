/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_kernel_execution.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiKernelExecution> registerTestCase{};

class MultiKernelExecutionTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, size_t, bool, bool>> {
};

TEST_P(MultiKernelExecutionTest, Test) {
    MultiKernelExecutionArguments args;
    args.api = Api::L0;
    args.kernelCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.workgroupCount = std::get<3>(GetParam());
    args.delay = std::get<4>(GetParam());
    args.inOrderOverOOO = std::get<5>(GetParam());
    args.profiling = std::get<6>(GetParam());

    MultiKernelExecution test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiKernelExecutionTest,
    MultiKernelExecutionTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(100u),
        ::testing::Values(32u),
        ::testing::Values(32u),
        ::testing::Values(1u),
        ::testing::Values(false),
        ::testing::Values(false, true)));
