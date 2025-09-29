/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/decoder2_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Decoder2Graph> registerTestCase{};

class Decoder2GraphTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, bool, bool, bool>> {};

TEST_P(Decoder2GraphTest, Test) {
    Decoder2GraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.numTokens = std::get<1>(GetParam());
    args.useGraphs = std::get<2>(GetParam());
    args.emulateGraphs = std::get<3>(GetParam());
    args.useHostTasks = std::get<4>(GetParam());
    Decoder2Graph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    Decoder2GraphTest, Decoder2GraphTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::L0),
        ::testing::Values(20, 40, 100, 200), // numTokens
        ::testing::Values(true, false),      // useGraphs
        ::testing::Values(true, false),      // emulateGraphs (L0 only)
        ::testing::Values(true, false)));    // useHostTasks
