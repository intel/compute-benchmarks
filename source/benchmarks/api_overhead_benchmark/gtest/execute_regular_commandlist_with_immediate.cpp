/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_regular_commandlist_with_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>
#include <tuple>

[[maybe_unused]] static const inline RegisterTestCase<ExecuteRegularCommandListWithImmediate> registerTestCase{};

class ExecuteRegularCommandListWithImmediateTest : public ::testing::TestWithParam<std::tuple<bool, bool, bool, bool, bool, bool>> {
};

TEST_P(ExecuteRegularCommandListWithImmediateTest, Test) {
    ExecuteRegularCommandListWithImmediateArguments args{};
    args.api = Api::L0;
    args.useEvent = std::get<0>(GetParam());
    args.useProfiling = std::get<1>(GetParam());
    args.measureCompletionTime = std::get<2>(GetParam());
    args.inOrder = std::get<3>(GetParam());
    args.counterBasedEvents = std::get<4>(GetParam());
    args.waitEvent = std::get<5>(GetParam());

    ExecuteRegularCommandListWithImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteRegularCommandListWithImmediateTest,
    ExecuteRegularCommandListWithImmediateTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(false),
        ::testing::Values(true, false),
        ::testing::Values(true, false),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
