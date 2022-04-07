/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/create_command_list_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<CreateCommandListImmediate> registerTestCase{};

class CreateCommandListImmediateTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(CreateCommandListImmediateTest, Test) {
    CreateCommandListImmediateArguments args{};
    args.api = Api::L0;
    args.cmdListCount = GetParam();
    CreateCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CreateCommandListImmediateTest,
    CreateCommandListImmediateTest,
    ::testing::Values(1, 10, 100, 1000));
