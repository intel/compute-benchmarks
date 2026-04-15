/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/unique_kernel_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UniqueKernelSwitchLatency> registerTestCase{};

class UniqueKernelSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, bool>> {
};

TEST_P(UniqueKernelSwitchLatencyTest, Test) {
    UniqueKernelSwitchLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelCount = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.workgroupSize = std::get<3>(GetParam());
    args.profiling = std::get<4>(GetParam());

    UniqueKernelSwitchLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    UniqueKernelSwitchLatencyTest,
    UniqueKernelSwitchLatencyTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(2, 4, 6),
        ::testing::Values(8),
        ::testing::Values(256),
        ::testing::Values(false, true)));
