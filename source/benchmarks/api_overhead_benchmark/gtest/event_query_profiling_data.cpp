/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_query_profiling_data.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EventQueryProfilingData> registerTestCase{};

class EventQueryProfilingDataTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, uint32_t>> {
};

TEST_P(EventQueryProfilingDataTest, Test) {
    EventQueryProfilingDataArguments args{};
    args.api = std::get<0>(GetParam());
    args.eventCount = std::get<1>(GetParam());
    args.splitCount = std::get<2>(GetParam());

    EventQueryProfilingData test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventQueryProfilingDataTest,
    EventQueryProfilingDataTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(10000u),
        ::testing::Values(1u, 2u, 3u)));
