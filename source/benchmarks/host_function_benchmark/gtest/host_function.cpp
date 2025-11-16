/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/host_function.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<HostFunction> registerTestCase{};

class HostFunctionTest : public ::testing::TestWithParam<std::tuple<Api, int, bool, int, bool, bool>> {
};

TEST_P(HostFunctionTest, Test) {
    HostFunctionArguments args{};
    args.api = std::get<0>(GetParam());
    args.amountOfCalls = std::get<1>(GetParam());
    args.measureCompletionTime = std::get<2>(GetParam());
    args.kernelExecutionTime = std::get<3>(GetParam());
    args.useIoq = std::get<4>(GetParam());
    args.useHostFunctionColdRun = std::get<5>(GetParam());

    HostFunction test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    HostFunctionTest,
    HostFunctionTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1u, 10u),
        ::testing::Values(false, true),
        ::testing::Values(1u, 100u),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
