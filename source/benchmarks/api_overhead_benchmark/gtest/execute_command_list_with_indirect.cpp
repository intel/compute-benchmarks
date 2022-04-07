/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/execute_command_list_with_indirect.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ExecuteCommandListWithInidrect> registerTestCase{};

class ExecuteCommandListTestWithIndirectTest : public ::testing::TestWithParam<std::tuple<size_t, UsmMemoryPlacement>> {
};

TEST_P(ExecuteCommandListTestWithIndirectTest, Test) {
    ExecuteCommandListWithIndirectArguments args{};
    args.api = Api::L0;
    args.IndirectAllocationsAmount = std::get<0>(GetParam());
    args.placement = std::get<1>(GetParam());

    ExecuteCommandListWithInidrect test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecuteCommandListTestWithIndirectTest,
    ExecuteCommandListTestWithIndirectTest,
    ::testing::Combine(
        ::testing::Values(0u, 10u, 100u, 1000u),
        ::testing::Values(UsmMemoryPlacement::Shared, UsmMemoryPlacement::Host)));
