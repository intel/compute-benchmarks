/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandList> registerTestCase{};

class ExecuteCommandListTest : public ::testing::TestWithParam<bool> {
};

TEST_P(ExecuteCommandListTest, Test) {
    ExecuteCommandListArguments args{};
    args.api = Api::L0;
    args.useFence = GetParam();
    args.measureCompletionTime = GetParam();

    ExecuteCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListTest,
    ExecuteCommandListTest,
    ::testing::Values(false, true, false));
