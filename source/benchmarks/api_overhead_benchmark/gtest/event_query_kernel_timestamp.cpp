/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_query_kernel_timestamp.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EventQueryKernelTimestamp> registerTestCase{};

class EventQueryKernelTimestampTest : public ::testing::TestWithParam<std::tuple<bool, uint32_t>> {
};

TEST_P(EventQueryKernelTimestampTest, Test) {
    EventQueryKernelTimestampArguments args{};
    args.api = Api::L0;
    args.useImmediate = std::get<0>(GetParam());
    args.queryCount = std::get<1>(GetParam());

    EventQueryKernelTimestamp test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventQueryKernelTimestampTest,
    EventQueryKernelTimestampTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(100u)));
