/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/immediate_cmdlist_completion.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiProcessImmediateCmdlistCompletion> registerTestCase{};

class MultiProcessImmediateCmdlistCompletionLatencyTest : public ::testing::TestWithParam<std::tuple<uint32_t, std::string, uint32_t, std::bitset<maxNumberOfEngines>>> {
};

TEST_P(MultiProcessImmediateCmdlistCompletionLatencyTest, Test) {
    MultiProcessImmediateCmdlistCompletionArguments args{};
    args.api = Api::L0;
    args.numberOfProcesses = std::get<0>(GetParam());
    args.engineGroup = std::get<1>(GetParam());
    args.copySize = std::get<2>(GetParam());
    args.engineMask = std::get<3>(GetParam());

    MultiProcessImmediateCmdlistCompletion test;
    test.run(args);
}

constexpr unsigned int megaByte = 1024 * 1024;
INSTANTIATE_TEST_SUITE_P(
    MultiProcessImmediateCmdlistCompletionLatencyTest,
    MultiProcessImmediateCmdlistCompletionLatencyTest,
    testing::Values(
        std::make_tuple(1, "Compute", 1 * megaByte, "1"),
        std::make_tuple(2, "Compute", 1 * megaByte, "1"),
        std::make_tuple(8, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(16, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(12, "Copy-Only", 1 * megaByte, "11111111"),
        std::make_tuple(7, "Copy-Only", 1 * megaByte, "11111110")));