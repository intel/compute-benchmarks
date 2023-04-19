/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_with_fence_usage.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandListWithFenceUsage> registerTestCase{};

class ExecuteCommandListTestWithFenceUsageTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(ExecuteCommandListTestWithFenceUsageTest, Test) {
    ExecuteCommandListWithFenceUsageArguments args{};
    args.api = Api::L0;

    ExecuteCommandListWithFenceUsage test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListTestWithFenceUsageTest,
    ExecuteCommandListTestWithFenceUsageTest,
    ::testing::Values(Api::L0));
