/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<QueueSwitch> registerTestCase{};

class QueueSwitchTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t>> {
};

TEST_P(QueueSwitchTest, Test) {
    QueueSwitchArguments args;
    args.api = Api::L0;
    args.kernelTime = std::get<1>(GetParam());
    args.switchCount = std::get<2>(GetParam());
    QueueSwitch test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueueSwitchTest,
    QueueSwitchTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(10u),
        ::testing::Values(1u)));
