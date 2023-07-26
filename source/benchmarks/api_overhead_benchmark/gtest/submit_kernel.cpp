/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitKernel> registerTestCase{};

class SubmitKernelTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, bool, size_t, size_t, bool>> {
};

TEST_P(SubmitKernelTest, Test) {
    SubmitKernelArguments args{};
    args.api = std::get<0>(GetParam());
    args.useProfiling = std::get<1>(GetParam());
    args.inOrderQueue = std::get<2>(GetParam());
    args.discardEvents = std::get<3>(GetParam());
    args.numKernels = std::get<4>(GetParam());
    args.kernelExecutionTime = std::get<5>(GetParam());
    args.measureCompletionTime = std::get<6>(GetParam());
    SubmitKernel test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SubmitKernelTest,
    SubmitKernelTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false),         // useProfiling
        ::testing::Values(false, true),   // inOrderQueue
        ::testing::Values(false, true),   // discardEvents
        ::testing::Values(10u),           // numKernels
        ::testing::Values(1u),            // kernelExecutionTime
        ::testing::Values(false, true))); // measureCompletionTime
