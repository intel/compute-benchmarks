/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_linear_kernel_size.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitLinearKernelSize> registerTestCase{};

class KernelSubmitLinearKernelSizeTest : public ::testing::TestWithParam<std::tuple<Api, int, int>> {
};

TEST_P(KernelSubmitLinearKernelSizeTest, Test) {
    KernelSubmitLinearKernelSizeArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelBatchSize = std::get<1>(GetParam());
    args.kernelSize = std::get<2>(GetParam());
    KernelSubmitLinearKernelSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitLinearKernelSizeTest,
    KernelSubmitLinearKernelSizeTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(512),
        ::testing::Values(32, 128, 512, 1024, 5120)));
