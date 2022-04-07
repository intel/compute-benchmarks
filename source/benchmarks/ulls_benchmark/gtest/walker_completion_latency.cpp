/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/walker_completion_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<WalkerCompletionLatency> registerTestCase{};

class WalkerCompletionLatencyTest : public ::testing::TestWithParam<Api> {
};

TEST_P(WalkerCompletionLatencyTest, Test) {
    WalkerCompletionLatencyArguments args{};
    args.api = GetParam();

    WalkerCompletionLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WalkerCompletionLatencyTest,
    WalkerCompletionLatencyTest,
    ::CommonGtestArgs::allApis());
