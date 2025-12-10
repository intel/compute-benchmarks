/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_switch_latency_fill.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSwitchLatencyFill> registerTestCase{};

class KernelSwitchLatencyFillTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, bool>> {
};

TEST_P(KernelSwitchLatencyFillTest, Test) {
    KernelSwitchLatencyFillArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelCount = std::get<1>(GetParam());
    args.fillSize = std::get<2>(GetParam());
    args.inOrder = std::get<3>(GetParam());

    KernelSwitchLatencyFill test;
    test.run(args);
}

using namespace MemoryConstants;

INSTANTIATE_TEST_SUITE_P(
    KernelSwitchLatencyFillTest,
    KernelSwitchLatencyFillTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(100),
        ::testing::Values(512 * kiloByte, 1 * megaByte, 2 * megaByte, 4 * megaByte, 8 * megaByte, 16 * megaByte),
        ::testing::Values(false, true)));