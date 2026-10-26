/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_get_ordinal.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListGetOrdinal> registerTestCase{};

class CommandListGetOrdinalTest : public ::testing::TestWithParam<bool> {
};

TEST_P(CommandListGetOrdinalTest, Test) {
    CommandListQueryArguments args{};
    args.api = Api::L0;
    args.useImmediate = GetParam();

    CommandListGetOrdinal test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListGetOrdinalTest,
    CommandListGetOrdinalTest,
    ::testing::Values(false, true));
