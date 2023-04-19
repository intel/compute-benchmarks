/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/wait_on_event_cold.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WaitOnEventCold> registerTestCase{};

class WaitOnEventColdTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(WaitOnEventColdTest, Test) {
    WaitOnEventColdArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    WaitOnEventCold test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WaitOnEventColdTest,
    WaitOnEventColdTest,
    ::testing::Values(500, 1000));
