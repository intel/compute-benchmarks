/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_graph_multi_queue.h"

#include "framework/enum/kernel_name.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitGraphMultiQueue> registerTestCase{};
class KernelSubmitGraphMultiQueueTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, uint32_t, uint32_t, bool, bool>> {
};

TEST_P(KernelSubmitGraphMultiQueueTest, Test) {
    KernelSubmitGraphMultiQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.workgroupCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.kernelsPerQueue = std::get<3>(GetParam());
    args.useProfiling = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());
    KernelSubmitGraphMultiQueue test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitGraphMultiQueueTest,
    KernelSubmitGraphMultiQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(512),           // workgroupCount
        ::testing::Values(256),           // workgroupSize
        ::testing::Values(1),             // kernelsPerQueue
        ::testing::Values(false),         // useProfiling
        ::testing::Values(false, true))); // useEvents
