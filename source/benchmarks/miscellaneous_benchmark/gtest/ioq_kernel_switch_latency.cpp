/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/ioq_kernel_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<IoqKernelSwitchLatency> registerTestCase{};

class IoqKernelSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool>> {
};

TEST_P(IoqKernelSwitchLatencyTest, Test) {
    IoqKernelSwitchLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelCount = std::get<1>(GetParam());
    args.useEvents = std::get<2>(GetParam());

    IoqKernelSwitchLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    IoqKernelSwitchLatencyTest,
    IoqKernelSwitchLatencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(32),
        ::testing::Bool()));
