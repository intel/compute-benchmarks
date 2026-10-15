/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_update_mutable_command_wait_events.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListUpdateMutableCommandWaitEvents> registerTestCase{};

class CommandListUpdateMutableCommandWaitEventsTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(CommandListUpdateMutableCommandWaitEventsTest, Test) {
    CommandListUpdateMutableCommandWaitEventsArguments args{};
    args.api = Api::L0;
    args.numWaitEvents = GetParam();

    CommandListUpdateMutableCommandWaitEvents test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListUpdateMutableCommandWaitEventsTest,
    CommandListUpdateMutableCommandWaitEventsTest,
    ::testing::Values(1u, 4u, 16u));
