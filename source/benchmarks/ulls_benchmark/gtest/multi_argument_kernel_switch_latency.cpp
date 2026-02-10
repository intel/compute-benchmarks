/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_argument_kernel_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiArgumentKernelSwitchLatency> registerTestCase{};

class MultiArgumentKernelSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, size_t, bool>> {
};

TEST_P(MultiArgumentKernelSwitchLatencyTest, Test) {
    MultiArgumentKernelSwitchLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelCount = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.workgroupSize = std::get<3>(GetParam());
    args.argumentCount = std::get<4>(GetParam());
    args.profiling = std::get<5>(GetParam());

    MultiArgumentKernelSwitchLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiArgumentKernelSwitchLatencyTest,
    MultiArgumentKernelSwitchLatencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(10, 100, 1000),
        ::testing::Values(8),
        ::testing::Values(256),
        ::testing::Values(1, 4, 8, 16, 32, 64),
        ::testing::Values(false, true)));
