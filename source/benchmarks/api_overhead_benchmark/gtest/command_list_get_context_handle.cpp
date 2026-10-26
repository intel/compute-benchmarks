/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_get_context_handle.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListGetContextHandle> registerTestCase{};

class CommandListGetContextHandleTest : public ::testing::TestWithParam<bool> {
};

TEST_P(CommandListGetContextHandleTest, Test) {
    CommandListQueryArguments args{};
    args.api = Api::L0;
    args.useImmediate = GetParam();

    CommandListGetContextHandle test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListGetContextHandleTest,
    CommandListGetContextHandleTest,
    ::testing::Values(false, true));
