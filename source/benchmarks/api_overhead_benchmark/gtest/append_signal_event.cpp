/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_signal_event.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendSignalEvent> registerTestCase{};

class AppendSignalEventTest : public ::testing::TestWithParam<std::tuple<uint32_t, bool, bool>> {
};

TEST_P(AppendSignalEventTest, Test) {
    AppendSignalEventArguments args{};
    args.api = Api::L0;
    args.appendCount = std::get<0>(GetParam());
    args.counterBasedEvents = std::get<1>(GetParam());
    args.timestampEvents = std::get<2>(GetParam());

    AppendSignalEvent test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendSignalEventTest,
    AppendSignalEventTest,
    ::testing::Values(
        std::make_tuple(100u, false, false),
        std::make_tuple(100u, false, true),
        std::make_tuple(100u, true, false)));
