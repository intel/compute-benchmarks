/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_graph_single_queue.h"

#include "framework/enum/kernel_name.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitGraphSingleQueue> registerTestCase{};
class KernelSubmitGraphSingleQueueTest : public ::testing::TestWithParam<std::tuple<Api, KernelName, uint32_t, uint32_t, uint32_t, size_t, bool, bool>> {
};

TEST_P(KernelSubmitGraphSingleQueueTest, Test) {
    KernelSubmitGraphSingleQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelName = std::get<1>(GetParam());
    args.kernelWGCount = std::get<2>(GetParam());
    args.kernelWGSize = std::get<3>(GetParam());
    args.kernelGroupsCount = std::get<4>(GetParam());
    args.kernelBatchSize = std::get<5>(GetParam());
    args.useProfiling = std::get<6>(GetParam());
    args.useEvents = std::get<7>(GetParam());
    KernelSubmitGraphSingleQueue test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitGraphSingleQueueTest,
    KernelSubmitGraphSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(KernelName::Empty, KernelName::Add, KernelName::AddSequence), // kernelName
        ::testing::Values(512),                                                         // kernelWGCount
        ::testing::Values(256),                                                         // kernelWGSize
        ::testing::Values(1),                                                           // kernelGroupsCount
        ::testing::Values(10),                                                          // kernelBatchSize
        ::testing::Values(false),                                                       // useProfiling
        ::testing::Values(false, true)));                                               // useEvents
