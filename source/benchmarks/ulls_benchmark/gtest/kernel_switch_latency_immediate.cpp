/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_switch_latency_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<KernelSwitchLatencyImmediate> registerTestCase{};

class KernelSwitchLatencyImmediateTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool>> {
};

TEST_P(KernelSwitchLatencyImmediateTest, Test) {
    KernelSwitchLatencyImmediateArguments args;
    args.api = Api::L0;
    args.kernelCount = std::get<1>(GetParam());
    args.barrier = std::get<2>(GetParam());
    args.hostVisible = std::get<3>(GetParam());

    KernelSwitchLatencyImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSwitchLatencyImmediateTest,
    KernelSwitchLatencyImmediateTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(8, 16, 32),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
