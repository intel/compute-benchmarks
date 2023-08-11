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

class DestroyCommandListImmediateTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(DestroyCommandListImmediateTest, Test) {
    DestroyCommandListImmediateArguments args{};
    args.api = Api::L0;
    args.cmdListCount = std::get<0>(GetParam());
    args.useIoq = std::get<1>(GetParam());
    DestroyCommandListImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DestroyCommandListImmediateTest,
    DestroyCommandListImmediateTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(100, 1000)));
