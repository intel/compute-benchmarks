/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/create_command_list_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CreateCommandListImmediate> registerTestCase{};

class CreateCommandListImmediateTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(CreateCommandListImmediateTest, Test) {
    CreateCommandListImmediateArguments args{};
    args.api = Api::L0;
    args.cmdListCount = std::get<0>(GetParam());
    args.useIoq = std::get<1>(GetParam());
    CreateCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CreateCommandListImmediateTest,
    CreateCommandListImmediateTest,
    ::testing::Combine(
        ::testing::Values(100, 1000),
        ::testing::Values(false, true)));
