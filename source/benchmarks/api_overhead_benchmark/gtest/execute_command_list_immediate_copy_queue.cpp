/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include "framework/enum/test_type.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/test_type_skip.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandListImmediateCopyQueue> registerTestCase{};

class ExecuteCommandListImmediateCopyQueueTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, UsmMemoryPlacement, UsmMemoryPlacement, size_t, bool, TestType>> {
};

TEST_P(ExecuteCommandListImmediateCopyQueueTest, Test) {
    ExecuteCommandListImmediateCopyQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.isCopyOnly = std::get<1>(GetParam());
    args.measureCompletionTime = std::get<2>(GetParam());
    args.sourcePlacement = std::get<3>(GetParam());
    args.destinationPlacement = std::get<4>(GetParam());
    args.size = std::get<5>(GetParam());
    args.useIoq = std::get<6>(GetParam());

    const auto testType = std::get<7>(GetParam());
    if (isTestSkipped(Configuration::get().reducedSizeCAL, testType)) {
        GTEST_SKIP();
    }

    ExecuteCommandListImmediateCopyQueue test;
    test.run(args);
}

constexpr static size_t megaByte = 1024 * 1024;

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListImmediateCopyQueueTest,
    ExecuteCommandListImmediateCopyQueueTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(true, false),
        ::testing::Values(false, true),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::limitedTargets),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::limitedTargets),
        ::testing::Values(64 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(TestType::Regular)));