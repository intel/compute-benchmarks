/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<KernelSwitchLatency> registerTestCase{};

class KernelSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool, bool>> {
};

TEST_P(KernelSwitchLatencyTest, Test) {
    KernelSwitchLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelCount = std::get<1>(GetParam());
    args.flushBetweenEnqueues = std::get<2>(GetParam());
    args.barrier = std::get<3>(GetParam());
    args.hostVisible = std::get<4>(GetParam());

    KernelSwitchLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSwitchLatencyTest,
    KernelSwitchLatencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(8, 32),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
