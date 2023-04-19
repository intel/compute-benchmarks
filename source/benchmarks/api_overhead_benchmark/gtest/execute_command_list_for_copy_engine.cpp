/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_for_copy_engine.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandListForCopyEngine> registerTestCase{};

class ExecuteCommandListForCopyEngineTest : public ::testing::TestWithParam<std::tuple<bool, bool>> {
};

TEST_P(ExecuteCommandListForCopyEngineTest, Test) {
    ExecuteCommandListArguments args{};
    args.api = Api::L0;
    args.useFence = std::get<0>(GetParam());
    args.measureCompletionTime = std::get<1>(GetParam());

    ExecuteCommandListForCopyEngine test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListForCopyEngineTest,
    ExecuteCommandListForCopyEngineTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
