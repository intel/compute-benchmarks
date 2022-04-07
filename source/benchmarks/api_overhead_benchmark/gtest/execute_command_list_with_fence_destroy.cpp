/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_with_fence_destroy.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ExecuteCommandListWithFenceDestroy> registerTestCase{};

class ExecuteCommandListTestWithFenceDestroyTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(ExecuteCommandListTestWithFenceDestroyTest, Test) {
    ExecuteCommandListWithFenceDestroyArguments args{};
    args.api = Api::L0;

    ExecuteCommandListWithFenceDestroy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListTestWithFenceDestroyTest,
    ExecuteCommandListTestWithFenceDestroyTest,
    ::testing::Values(Api::L0));
