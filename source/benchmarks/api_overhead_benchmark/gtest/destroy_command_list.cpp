/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/destroy_command_list.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DestroyCommandList> registerTestCase{};

class DestroyCommandListTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(DestroyCommandListTest, Test) {
    DestroyCommandListArguments args{};
    args.api = Api::L0;
    args.cmdListCount = GetParam();

    DestroyCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DestroyCommandListTest,
    DestroyCommandListTest,
    ::testing::Values(100, 1000));
