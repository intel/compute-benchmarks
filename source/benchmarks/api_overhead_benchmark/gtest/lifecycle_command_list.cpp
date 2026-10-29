/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/lifecycle_command_list.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<LifecycleCommandList> registerTestCase{};

class LifecycleCommandListTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool>> {
};

TEST_P(LifecycleCommandListTest, Test) {
    LifecycleCommandListArguments args{};
    args.api = std::get<0>(GetParam());
    args.cmdListCount = std::get<1>(GetParam());
    args.copyOnly = std::get<2>(GetParam());
    LifecycleCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    LifecycleCommandListTest,
    LifecycleCommandListTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::Vulkan),
        ::testing::Values(1, 10, 100),
        ::testing::Values(false, true)));
