/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ExecuteCommandListImmediate> registerTestCase{};

class ExecuteCommandListImmediateTest : public ::testing::TestWithParam<std::tuple<bool, size_t, bool, size_t, bool>> {
};

TEST_P(ExecuteCommandListImmediateTest, Test) {
    ExecuteCommandListImmediateArguments args{};
    args.api = Api::L0;
    args.useProfiling = std::get<0>(GetParam());
    args.amountOfCalls = std::get<1>(GetParam());
    args.measureCompletionTime = std::get<2>(GetParam());
    args.kernelExecutionTime = std::get<3>(GetParam());
    args.useBarrierSynchronization = std::get<4>(GetParam());
    ExecuteCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListImmediateTest,
    ExecuteCommandListImmediateTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(1u, 10u),
        ::testing::Values(false, true),
        ::testing::Values(1u, 10u, 100u),
        ::testing::Values(false, true)));
