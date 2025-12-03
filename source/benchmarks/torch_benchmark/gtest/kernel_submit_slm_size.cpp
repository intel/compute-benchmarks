/*
 * Copyright (C) 2025 Intel Corporation
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

class KernelSubmitSlmSizeTest : public ::testing::TestWithParam<std::tuple<Api, int, int, int>> {
};

TEST_P(KernelSubmitSlmSizeTest, Test) {
    KernelSubmitSlmSizeArguments args{};
    args.api = std::get<0>(GetParam());
    args.batchSize = std::get<1>(GetParam());
    args.slmNum = std::get<2>(GetParam());
    args.warmupIterations = std::get<3>(GetParam());
    KernelSubmitSlmSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSlmSizeTest,
    KernelSubmitSlmSizeTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(512),
        ::testing::Values(-1),
        ::testing::Values(2)));
