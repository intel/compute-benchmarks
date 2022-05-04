/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_immediate_multi_kernel.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ExecuteCommandListImmediateMultiKernel> registerTestCase{};

class ExecuteCommandListImmediateMultiKernelTest : public ::testing::TestWithParam<std::tuple<size_t, size_t>> {
};

TEST_P(ExecuteCommandListImmediateMultiKernelTest, Test) {
    ExecuteCommandListImmediateMultiKernelArguments args{};
    args.api = Api::L0;
    args.amountOfCalls = std::get<0>(GetParam());
    args.kernelExecutionTime = std::get<1>(GetParam());

    ExecuteCommandListImmediateMultiKernel test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListImmediateMultiKernelTest,
    ExecuteCommandListImmediateMultiKernelTest,
    ::testing::Combine(
        ::testing::Values(1u, 2u, 4u, 8u, 16u),
        ::testing::Values(1u, 10u, 100u)));
