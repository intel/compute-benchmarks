/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/destroy_command_list_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DestroyCommandListImmediate> registerTestCase{};

class DestroyCommandListImmediateTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(DestroyCommandListImmediateTest, Test) {
    DestroyCommandListImmediateArguments args{};
    args.api = Api::L0;
    args.cmdListCount = GetParam();
    DestroyCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DestroyCommandListImmediateTest,
    DestroyCommandListImmediateTest,
    ::testing::Values(100, 1000));
