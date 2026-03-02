/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_graph_vllm_mock.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitGraphVllmMock> registerTestCase{};

class KernelSubmitGraphVllmMockTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, uint32_t, uint32_t, uint32_t, bool, bool>> {
};

TEST_P(KernelSubmitGraphVllmMockTest, Test) {
    KernelSubmitGraphVllmMockArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelWGCount = std::get<1>(GetParam());
    args.kernelWGSize = std::get<2>(GetParam());
    args.allocCount = std::get<3>(GetParam());
    args.graphScenario = std::get<4>(GetParam());
    args.useProfiling = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());
    KernelSubmitGraphVllmMock test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitGraphVllmMockTest,
    KernelSubmitGraphVllmMockTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(512),           // kernelWGCount
        ::testing::Values(256),           // kernelWGSize
        ::testing::Values(32),            // allocCount
        ::testing::Values(0, 1, 2, 3),    // graphScenario
        ::testing::Values(false),         // useProfiling
        ::testing::Values(false, true))); // useEvents
