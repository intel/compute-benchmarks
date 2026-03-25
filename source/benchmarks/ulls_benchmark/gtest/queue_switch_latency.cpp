/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<QueueSwitch> registerTestCase{};

class QueueSwitchTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, size_t, size_t, bool>> {
};

TEST_P(QueueSwitchTest, Test) {
    QueueSwitchArguments args;
    args.api = std::get<0>(GetParam());
    args.kernelTime = std::get<1>(GetParam());
    args.switchCount = std::get<2>(GetParam());
    args.queuesCount = std::get<3>(GetParam());
    args.workgroupCount = std::get<4>(GetParam());
    args.workgroupSize = std::get<5>(GetParam());
    args.differentKernels = std::get<6>(GetParam());
    QueueSwitch test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueueSwitchTest,
    QueueSwitchTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(10u),
        ::testing::Values(1u),
        ::testing::Values(1u, 2u),
        ::testing::Values(32u),
        ::testing::Values(32u),
        ::testing::Values(true, false)));

INSTANTIATE_TEST_SUITE_P(
    QueueSwitchTestLIMITED,
    QueueSwitchTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(10u),
        ::testing::Values(1u),
        ::testing::Values(1u),
        ::testing::Values(32u),
        ::testing::Values(32u),
        ::testing::Values(false)));
