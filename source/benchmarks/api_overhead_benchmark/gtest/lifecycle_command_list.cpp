/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/lifecycle_command_list.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<LifecycleCommandList> registerTestCase{};

class LifecycleCommandListTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(LifecycleCommandListTest, Test) {
    LifecycleCommandListArguments args{};
    args.api = Api::L0;
    args.cmdListCount = std::get<0>(GetParam());
    args.copyOnly = std::get<1>(GetParam());
    LifecycleCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    LifecycleCommandListTest,
    LifecycleCommandListTest,
    ::testing::Combine(
        ::testing::Values(1, 10, 100),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    LifecycleCommandListTestLIMITED,
    LifecycleCommandListTest,
    ::testing::Combine(
        ::testing::Values(100),
        ::testing::Values(false)));
