/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_host_synchronize.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListHostSynchronize> registerTestCase{};

class CommandListHostSynchronizeTest : public ::testing::TestWithParam<bool> {
};

TEST_P(CommandListHostSynchronizeTest, Test) {
    CommandListHostSynchronizeArguments args{};
    args.api = Api::L0;
    args.useBarrierBeforeSync = GetParam();

    CommandListHostSynchronize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListHostSynchronizeTest,
    CommandListHostSynchronizeTest,
    ::testing::Values(true, false));
