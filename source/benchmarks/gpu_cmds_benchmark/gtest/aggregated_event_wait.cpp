/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/aggregated_event_wait.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AggregatedEventWait> registerTestCase{};

class AggregatedEventWaitTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AggregatedEventWaitTest, Test) {
    AggregatedEventWaitArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    AggregatedEventWait test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AggregatedEventWaitTest,
    AggregatedEventWaitTest,
    ::testing::Values(100, 1000));

INSTANTIATE_TEST_SUITE_P(
    AggregatedEventWaitTestLIMITED,
    AggregatedEventWaitTest,
    ::testing::Values(100));
