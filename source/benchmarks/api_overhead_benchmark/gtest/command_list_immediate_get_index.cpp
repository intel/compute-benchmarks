/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_immediate_get_index.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListImmediateGetIndex> registerTestCase{};

class CommandListImmediateGetIndexTest : public ::testing::TestWithParam<Api> {
};

TEST_P(CommandListImmediateGetIndexTest, Test) {
    CommandListImmediateGetIndexArguments args{};
    args.api = GetParam();

    CommandListImmediateGetIndex test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListImmediateGetIndexTest,
    CommandListImmediateGetIndexTest,
    ::testing::Values(Api::L0));
