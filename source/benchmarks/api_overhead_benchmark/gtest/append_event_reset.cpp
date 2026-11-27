/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_event_reset.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendEventReset> registerTestCase{};

class AppendEventResetTest : public ::testing::TestWithParam<std::tuple<uint32_t, bool>> {
};

TEST_P(AppendEventResetTest, Test) {
    AppendEventResetArguments args{};
    args.api = Api::L0;
    args.appendCount = std::get<0>(GetParam());
    args.timestampEvents = std::get<1>(GetParam());

    AppendEventReset test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendEventResetTest,
    AppendEventResetTest,
    ::testing::Values(
        std::make_tuple(100u, false),
        std::make_tuple(100u, true)));
