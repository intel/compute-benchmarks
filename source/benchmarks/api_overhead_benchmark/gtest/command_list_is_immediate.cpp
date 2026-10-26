/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_is_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListIsImmediate> registerTestCase{};

class CommandListIsImmediateTest : public ::testing::TestWithParam<bool> {
};

TEST_P(CommandListIsImmediateTest, Test) {
    CommandListQueryArguments args{};
    args.api = Api::L0;
    args.useImmediate = GetParam();

    CommandListIsImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListIsImmediateTest,
    CommandListIsImmediateTest,
    ::testing::Values(false, true));
