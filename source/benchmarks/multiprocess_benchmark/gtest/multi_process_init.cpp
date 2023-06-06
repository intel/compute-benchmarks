/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_process_init.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiProcessInit> registerTestCase{};

class MultiProcessInitOverheadTest : public ::testing::TestWithParam<std::tuple<uint32_t, int32_t>> {
};

TEST_P(MultiProcessInitOverheadTest, Test) {
    MultiProcessInitArguments args{};
    args.api = Api::L0;
    args.numberOfProcesses = std::get<0>(GetParam());
    args.initFlag = std::get<1>(GetParam());

    MultiProcessInit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiProcessInitOverheadTest,
    MultiProcessInitOverheadTest,
    ::testing::Combine(
        ::testing::Values(1, 2, 4, 8, 16, 32, 64), ::testing::Values(0, 1, 2)));
