/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_switch_latency_priority_imm.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSwitchPriorityImm> registerTestCase{};

class KernelSwitchPriorityImmTest : public ::testing::TestWithParam<std::tuple<Api, PriorityLevel, PriorityLevel, size_t, size_t, bool>> {
};

TEST_P(KernelSwitchPriorityImmTest, Test) {
    KernelSwitchPriorityImmArguments args;
    args.api = std::get<0>(GetParam());
    args.measuredQueuePriority = std::get<1>(GetParam());
    args.secondaryQueuePriority = std::get<2>(GetParam());
    args.kernelCount = std::get<3>(GetParam());
    args.kernelExecutionTime = std::get<4>(GetParam());
    args.useIoq = std::get<5>(GetParam());

    KernelSwitchPriorityImm test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSwitchPriorityImmTest,
    KernelSwitchPriorityImmTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(PriorityLevel::Normal, PriorityLevel::High),
        ::testing::Values(PriorityLevel::Normal, PriorityLevel::High),
        ::testing::Values(8),
        ::testing::Values(200u),
        ::testing::Values(false, true)));
