/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitGraph> registerTestCase{};

class SubmitGraphTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, bool, bool, size_t, size_t, bool>> {
};

TEST_P(SubmitGraphTest, Test) {
    SubmitGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.useProfiling = std::get<1>(GetParam());
    args.inOrderQueue = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    args.useExplicit = std::get<4>(GetParam());
    args.numKernels = std::get<5>(GetParam());
    args.kernelExecutionTime = std::get<6>(GetParam());
    args.measureCompletionTime = std::get<7>(GetParam());
    SubmitGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SubmitGraphTest,
    SubmitGraphTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false, true),      // useProfiling
        ::testing::Values(false, true),      // inOrderQueue
        ::testing::Values(false, true),      // useEvents
        ::testing::Values(false, true),      // useExplicit
        ::testing::Values(4u, 8u, 16u, 32u), // numKernels
        ::testing::Values(1u),               // kernelExecutionTime
        ::testing::Values(false, true)));    // measureCompletionTime
