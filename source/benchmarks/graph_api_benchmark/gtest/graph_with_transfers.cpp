/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/graph_with_transfers.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<GraphWithTransfers> registerTestCase{};

class GraphWithTransfersTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool, size_t>> {
};

TEST_P(GraphWithTransfersTest, Test) {
    GraphWithTransfersArguments args{};
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.withCopyOffload = std::get<2>(GetParam());
    args.withComputeGraph = std::get<3>(GetParam());
    args.computeIterations = std::get<4>(GetParam());
    GraphWithTransfers test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    GraphWithTransfersTest,
    GraphWithTransfersTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(32 * MemoryConstants::megaByte, 64 * MemoryConstants::megaByte, 256 * MemoryConstants::megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(1024)));
