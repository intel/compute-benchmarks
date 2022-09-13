/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_query_status.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<EventQueryStatus> registerTestCase{};

class EventQueryStatusTest : public ::testing::TestWithParam<std::tuple<bool>> {
};

TEST_P(EventQueryStatusTest, Test) {
    EventQueryStatusArguments args{};
    args.api = Api::L0;
    args.eventSignaled = std::get<0>(GetParam());
    EventQueryStatus test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventQueryStatusTest,
    EventQueryStatusTest,
    ::testing::Combine(
        ::testing::Values(false, true)));
