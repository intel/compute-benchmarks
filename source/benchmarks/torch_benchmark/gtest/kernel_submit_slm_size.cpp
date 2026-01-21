/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_slm_size.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitSlmSize> registerTestCase{};

class KernelSubmitSlmSizeTest : public ::testing::TestWithParam<std::tuple<Api, int, int, bool, bool>> {
};

TEST_P(KernelSubmitSlmSizeTest, Test) {
    KernelSubmitSlmSizeArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelBatchSize = std::get<1>(GetParam());
    args.slmNum = std::get<2>(GetParam());
    args.useProfiling = std::get<3>(GetParam());
    args.measureCompletion = std::get<4>(GetParam());
    KernelSubmitSlmSize().run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSlmSizeTest,
    KernelSubmitSlmSizeTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(512),           // kernelBatchSize
        ::testing::Values(1024),          // slmNum
        ::testing::Values(false),         // useProfiling
        ::testing::Values(false, true))); // measureCompletion
