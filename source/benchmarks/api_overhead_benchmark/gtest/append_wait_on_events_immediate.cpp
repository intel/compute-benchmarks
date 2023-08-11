/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_wait_on_events_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendWaitOnEventsImmediate> registerTestCase{};

class AppendWaitOnEventsImmediateTest : public ::testing::TestWithParam<std::tuple<bool, bool>> {
};

TEST_P(AppendWaitOnEventsImmediateTest, Test) {
    AppendWaitOnEventsImmediateArguments args{};
    args.api = Api::L0;
    args.eventSignaled = std::get<0>(GetParam());
    args.useIoq = std::get<1>(GetParam());
    AppendWaitOnEventsImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendWaitOnEventsImmediateTest,
    AppendWaitOnEventsImmediateTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
