/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandListImmediate> registerTestCase{};

class ExecuteCommandListImmediateTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t, bool, size_t, bool, bool, bool>> {
};

TEST_P(ExecuteCommandListImmediateTest, Test) {
    ExecuteCommandListImmediateArguments args{};
    args.api = std::get<0>(GetParam());
    args.useProfiling = std::get<1>(GetParam());
    args.amountOfCalls = std::get<2>(GetParam());
    args.measureCompletionTime = std::get<3>(GetParam());
    args.kernelExecutionTime = std::get<4>(GetParam());
    args.useBarrierSynchronization = std::get<5>(GetParam());
    args.useEventForHostSync = std::get<6>(GetParam());
    args.useIoq = std::get<7>(GetParam());
    ExecuteCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListImmediateTest,
    ExecuteCommandListImmediateTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false, true),
        ::testing::Values(1u, 10u),
        ::testing::Values(false, true),
        ::testing::Values(1u, 100u),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
