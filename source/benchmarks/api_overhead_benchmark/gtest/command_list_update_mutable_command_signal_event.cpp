/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_update_mutable_command_signal_event.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListUpdateMutableCommandSignalEvent> registerTestCase{};

class CommandListUpdateMutableCommandSignalEventTest : public ::testing::Test {
};

TEST_F(CommandListUpdateMutableCommandSignalEventTest, Test) {
    CommandListUpdateMutableCommandSignalEventArguments args{};
    args.api = Api::L0;

    CommandListUpdateMutableCommandSignalEvent test;
    test.run(args);
}
