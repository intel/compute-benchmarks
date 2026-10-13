/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_reuse_imm_cmdlist_in_progress.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EventReuseImmCmdlistInProgress> registerTestCase{};

class EventReuseImmCmdlistInProgressTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(EventReuseImmCmdlistInProgressTest, Test) {
    EventReuseImmCmdlistInProgressArguments args{};
    args.api = Api::L0;
    args.kernelExecutionTime = GetParam();

    EventReuseImmCmdlistInProgress test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventReuseImmCmdlistInProgressTest,
    EventReuseImmCmdlistInProgressTest,
    ::testing::Values(100));
