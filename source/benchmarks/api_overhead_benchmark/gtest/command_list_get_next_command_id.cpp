/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_get_next_command_id.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListGetNextCommandId> registerTestCase{};
class CommandListGetNextCommandIdTest : public ::testing::TestWithParam<MutableCommandFlag> {
};

TEST_P(CommandListGetNextCommandIdTest, Test) {
    CommandListGetNextCommandIdArguments args{};
    args.api = Api::L0;
    args.mutableCommandFlag = GetParam();

    CommandListGetNextCommandId test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListGetNextCommandIdTest,
    CommandListGetNextCommandIdTest,
    ::testing::Values(MutableCommandFlag::All, // mutableCommandFlag
                      MutableCommandFlag::KernelArguments,
                      MutableCommandFlag::GroupCount,
                      MutableCommandFlag::GroupSize,
                      MutableCommandFlag::GlobalOffset,
                      MutableCommandFlag::SignalEvent,
                      MutableCommandFlag::WaitEvents,
                      MutableCommandFlag::GraphArguments));