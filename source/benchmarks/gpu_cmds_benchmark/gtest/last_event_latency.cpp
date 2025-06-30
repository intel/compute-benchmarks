/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/last_event_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<LastEventLatency> registerTestCase{};

class LastEventLatencyTest : public ::testing::TestWithParam<bool> {
};

TEST_P(LastEventLatencyTest, Test) {
    LastEventLatencyArguments args{};
    args.api = Api::L0;
    args.signalOnBarrier = GetParam();

    LastEventLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    LastEventLatencyTest,
    LastEventLatencyTest,
    ::testing::Values(false, true));
