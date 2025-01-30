/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/wait_on_event_hot.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WaitOnEventHot> registerTestCase{};

class WaitOnEventHotTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(WaitOnEventHotTest, Test) {
    WaitOnEventHotArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    WaitOnEventHot test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WaitOnEventHotTest,
    WaitOnEventHotTest,
    ::testing::Values(500, 1000));

INSTANTIATE_TEST_SUITE_P(
    WaitOnEventHotTestLIMITED,
    WaitOnEventHotTest,
    ::testing::Values(500));
