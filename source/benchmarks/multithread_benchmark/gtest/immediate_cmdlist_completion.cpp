/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/immediate_cmdlist_completion.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ImmediateCommandListCompletion> registerTestCase{};

class ImmediateCommandListCompletionLatencyTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t, std::string, uint32_t, std::bitset<maxNumberOfEngines>>> {
};

TEST_P(ImmediateCommandListCompletionLatencyTest, Test) {
    ImmediateCommandListCompletionArguments args{};
    args.api = Api::L0;
    args.numberOfThreads = std::get<0>(GetParam());
    args.threadsPerEngine = std::get<1>(GetParam());
    args.engineGroup = std::get<2>(GetParam());
    args.copySize = std::get<3>(GetParam());
    args.engineMask = std::get<4>(GetParam());

    ImmediateCommandListCompletion test;
    test.run(args);
}
constexpr unsigned int megaByte = 1024 * 1024;

INSTANTIATE_TEST_SUITE_P(
    ImmediateCommandListCompletionLatencyTest,
    ImmediateCommandListCompletionLatencyTest,
    testing::Values(
        std::make_tuple(1, 1, "Compute", 1 * megaByte, "1"),
        std::make_tuple(2, 2, "Compute", 1 * megaByte, "1"),
        std::make_tuple(8, 1, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(16, 2, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(12, 1, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(8, 1, "Copy-Only", 1 * megaByte, "11111110")));
