/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_with_fence_create.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ExecuteCommandListWithFenceCreate> registerTestCase{};

class ExecuteCommandListTestWithFenceCreateTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(ExecuteCommandListTestWithFenceCreateTest, Test) {
    ExecuteCommandListWithFenceCreateArguments args{};
    args.api = Api::L0;

    ExecuteCommandListWithFenceCreate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListTestWithFenceCreateTest,
    ExecuteCommandListTestWithFenceCreateTest,
    ::testing::Values(Api::L0));
