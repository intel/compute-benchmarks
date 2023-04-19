/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/wait_on_event_from_walker.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WaitOnEventFromWalker> registerTestCase{};

class WaitOnEventFromWalkerTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(WaitOnEventFromWalkerTest, Test) {
    WaitOnEventFromWalkerArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    WaitOnEventFromWalker test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WaitOnEventFromWalkerTest,
    WaitOnEventFromWalkerTest,
    ::testing::Values(500, 1000));
