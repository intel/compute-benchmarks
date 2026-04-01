/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/aggregated_event_signaling.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AggregatedEventSignaling> registerTestCase{};

class AggregatedEventSignalingTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AggregatedEventSignalingTest, Test) {
    AggregatedEventSignalingArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    AggregatedEventSignaling test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AggregatedEventSignalingTest,
    AggregatedEventSignalingTest,
    ::testing::Values(100, 1000));

INSTANTIATE_TEST_SUITE_P(
    AggregatedEventSignalingTestLIMITED,
    AggregatedEventSignalingTest,
    ::testing::Values(100));
