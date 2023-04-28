/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteCommandListImmediateCopyQueue> registerTestCase{};

class ExecuteCommandListImmediateCopyQueueTest : public ::testing::TestWithParam<std::tuple<bool, bool, UsmMemoryPlacement, UsmMemoryPlacement, size_t>> {
};

TEST_P(ExecuteCommandListImmediateCopyQueueTest, Test) {
    ExecuteCommandListImmediateCopyQueueArguments args{};
    args.api = Api::L0;
    args.isCopyOnly = std::get<0>(GetParam());
    args.measureCompletionTime = std::get<1>(GetParam());
    args.sourcePlacement = std::get<2>(GetParam());
    args.destinationPlacement = std::get<3>(GetParam());
    args.size = std::get<4>(GetParam());
    ExecuteCommandListImmediateCopyQueue test;
    test.run(args);
}

constexpr static size_t megaByte = 1024 * 1024;

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListImmediateCopyQueueTest,
    ExecuteCommandListImmediateCopyQueueTest,
    ::testing::Combine(
        ::testing::Values(true, false),
        ::testing::Values(false, true),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::enumValues),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::limitedTargets),
        ::testing::Values(512 * megaByte)));
