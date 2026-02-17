/*
 * Copyright (C) 2025-2026 Intel Corporation
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

class KernelSubmitMultiQueueTest : public ::testing::TestWithParam<std::tuple<Api, int, int, int, bool, bool, bool>> {
};

TEST_P(KernelSubmitMultiQueueTest, Test) {
    KernelSubmitMultiQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelWGCount = std::get<1>(GetParam());
    args.kernelWGSize = std::get<2>(GetParam());
    args.kernelsPerQueue = std::get<3>(GetParam());
    args.useProfiling = std::get<4>(GetParam());
    args.measureCompletion = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());
    KernelSubmitMultiQueue().run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitMultiQueueTest,
    KernelSubmitMultiQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::SYCL, Api::SYCLPREVIEW),
        ::testing::Values(512),           // kernelWGCount
        ::testing::Values(256),           // kernelWGSize
        ::testing::Values(10),            // kernelsPerQueue
        ::testing::Values(false),         // useProfiling
        ::testing::Values(false, true),   // measureCompletion
        ::testing::Values(false, true))); // useEvents
