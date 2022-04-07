/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/create_command_list.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<CreateCommandList> registerTestCase{};

class CreateCommandListTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(CreateCommandListTest, Test) {
    CreateCommandListArguments args{};
    args.api = Api::L0;
    args.cmdListCount = std::get<0>(GetParam());
    args.copyOnly = std::get<1>(GetParam());
    CreateCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CreateCommandListTest,
    CreateCommandListTest,
    ::testing::Combine(
        ::testing::Values(1, 10, 100),
        ::testing::Values(false, true)));
