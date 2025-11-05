/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_multi_queue.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitMultiQueue> registerTestCase{};

class KernelSubmitMultiQueueTest : public ::testing::TestWithParam<std::tuple<Api, int, int, int>> {
};

TEST_P(KernelSubmitMultiQueueTest, Test) {
    KernelSubmitMultiQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.workgroupCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.kernelsPerQueue = std::get<3>(GetParam());
    KernelSubmitMultiQueue test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitMultiQueueTest,
    KernelSubmitMultiQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(512),
        ::testing::Values(256),
        ::testing::Values(10)));
