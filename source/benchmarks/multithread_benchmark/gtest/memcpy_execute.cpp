/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/memcpy_execute.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MemcpyExecute> registerTestCase{};

class MemcpyExecuteTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t, size_t, size_t, bool, bool, bool>> {
};

TEST_P(MemcpyExecuteTest, Test) {
    MemcpyExecuteArguments args{};
    args.api = std::get<0>(GetParam());
    args.inOrderQueue = std::get<1>(GetParam());
    args.numOpsPerThread = std::get<2>(GetParam());
    args.numThreads = std::get<3>(GetParam());
    args.allocSize = std::get<4>(GetParam());
    args.measureCompletionTime = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());
    args.useQueuePerThread = std::get<7>(GetParam());
    MemcpyExecute test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MemcpyExecuteTest,
    MemcpyExecuteTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(true),          // inOrderQueue
        ::testing::Values(1u, 100u),      // numOpsPerThread
        ::testing::Values(4u),            // numThreads
        ::testing::Values(1024u * 1024u), // allocSize
        ::testing::Values(false, true),   // measureCompletionTime
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(true)           // useQueuePerThread
        ));
