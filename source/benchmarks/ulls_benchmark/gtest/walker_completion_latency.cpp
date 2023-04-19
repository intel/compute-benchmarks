/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/walker_completion_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WalkerCompletionLatency> registerTestCase{};

class WalkerCompletionLatencyTest : public ::testing::TestWithParam<std::tuple<Api, bool>> {
};

TEST_P(WalkerCompletionLatencyTest, Test) {
    WalkerCompletionLatencyArguments args{};
    args.api = std::get<0>(GetParam());
    args.useFence = std::get<1>(GetParam());

    WalkerCompletionLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WalkerCompletionLatencyTest,
    WalkerCompletionLatencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(true, false)));
