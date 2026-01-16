/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_queue_execution.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiQueueExecution> registerTestCase{};

class MultiQueueExecutionTest : public ::testing::TestWithParam<std::tuple<Api, PriorityLevel, PriorityLevel, size_t, bool, bool>> {
};

TEST_P(MultiQueueExecutionTest, Test) {
    MultiQueueExecutionArguments args;
    args.api = std::get<0>(GetParam());
    args.measuredQueuePriority = std::get<1>(GetParam());
    args.secondaryQueuePriority = std::get<2>(GetParam());
    args.kernelCount = std::get<3>(GetParam());
    args.useSecondaryQueue = std::get<4>(GetParam());
    args.useIoq = std::get<5>(GetParam());

    MultiQueueExecution test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiQueueExecutionTest,
    MultiQueueExecutionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::ValuesIn(PriorityLevelArgument::enumValues),
        ::testing::ValuesIn(PriorityLevelArgument::enumValues),
        ::testing::Values(8),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
