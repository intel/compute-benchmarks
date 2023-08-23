/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_time.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EventTime> registerTestCase{};

class EventTimeTest : public ::testing::TestWithParam<std::tuple<bool, bool, EventScope, EventScope, uint32_t>> {
};

TEST_P(EventTimeTest, Test) {
    EventTimeArguments args{};
    args.api = Api::L0;
    args.useProfiling = std::get<0>(GetParam());
    args.hostVisible = std::get<1>(GetParam());
    args.signalScope = std::get<2>(GetParam());
    args.waitScope = std::get<3>(GetParam());
    args.eventCount = std::get<4>(GetParam());

    EventTime test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventTimeTest,
    EventTimeTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::ValuesIn(EventScopeArgument::enumValues),
        ::testing::ValuesIn(EventScopeArgument::enumValues),
        ::testing::Values(100u)));
