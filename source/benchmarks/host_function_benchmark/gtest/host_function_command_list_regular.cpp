/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/host_function_command_list_regular.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<HostFunctionCommandListRegular> registerTestCase{};

class HostFunctionCommandListRegularTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, bool, int, int>> {
};

TEST_P(HostFunctionCommandListRegularTest, Test) {
    HostFunctionCommandListRegularArguments args{};
    args.api = std::get<0>(GetParam());
    args.measureCompletionTime = std::get<1>(GetParam());
    args.useEmptyHostFunction = std::get<2>(GetParam());
    args.useKernels = std::get<3>(GetParam());
    args.kernelExecutionTime = std::get<4>(GetParam());
    args.amountOfCalls = std::get<5>(GetParam());

    HostFunctionCommandListRegular test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ScheduleOverhead,
    HostFunctionCommandListRegularTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),   // api
        ::testing::Values(false),     // measureCompletionTime
        ::testing::Values(true),      // useEmptyHostFunction
        ::testing::Values(false),     // useKernels
        ::testing::Values(1u),        // kernelExecutionTime
        ::testing::Values(1u, 10u))); // amountOfCalls

INSTANTIATE_TEST_SUITE_P(
    EmptyHostFunction,
    HostFunctionCommandListRegularTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),   // api
        ::testing::Values(true),      // measureCompletionTime
        ::testing::Values(true),      // useEmptyHostFunction
        ::testing::Values(false),     // useKernels
        ::testing::Values(1u),        // kernelExecutionTime
        ::testing::Values(1u, 10u))); // amountOfCalls

INSTANTIATE_TEST_SUITE_P(
    EmptyHostFunctionWithKernels,
    HostFunctionCommandListRegularTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),   // api
        ::testing::Values(true),      // measureCompletionTime
        ::testing::Values(true),      // useEmptyHostFunction
        ::testing::Values(true),      // useKernels
        ::testing::Values(1u, 1000u), // kernelExecutionTime
        ::testing::Values(1u, 10u))); // amountOfCalls

INSTANTIATE_TEST_SUITE_P(
    BusySpinFor1msHostFunction,
    HostFunctionCommandListRegularTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),   // api
        ::testing::Values(true),      // measureCompletionTime
        ::testing::Values(false),     // useEmptyHostFunction
        ::testing::Values(false),     // useKernels
        ::testing::Values(1u),        // kernelExecutionTime
        ::testing::Values(1u, 10u))); // amountOfCalls

INSTANTIATE_TEST_SUITE_P(
    BusySpinFor1msHostFunctionWithKernels,
    HostFunctionCommandListRegularTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),   // api
        ::testing::Values(true),      // measureCompletionTime
        ::testing::Values(false),     // useEmptyHostFunction
        ::testing::Values(true),      // useKernels
        ::testing::Values(1u, 1000u), // kernelExecutionTime
        ::testing::Values(1u, 10u))); // amountOfCalls
