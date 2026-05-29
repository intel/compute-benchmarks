/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_update_mutable_commands.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListUpdateMutableCommands> registerTestCase{};

class CommandListUpdateMutableCommandsTest : public ::testing::TestWithParam<MutableCommandFlag> {
};

TEST_P(CommandListUpdateMutableCommandsTest, Test) {
    CommandListUpdateMutableCommandsArguments args{};
    args.api = Api::L0;
    args.mutableCommandFlag = GetParam();

    CommandListUpdateMutableCommands test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListUpdateMutableCommandsTest,
    CommandListUpdateMutableCommandsTest,
    ::testing::Values(MutableCommandFlag::All, // mutableCommandFlag
                      MutableCommandFlag::KernelArguments,
                      MutableCommandFlag::GroupCount,
                      MutableCommandFlag::GroupSize,
                      MutableCommandFlag::GlobalOffset));
