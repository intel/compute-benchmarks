/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/graph_replay_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<GraphReplayLatency> registerTestCase{};

class GraphReplayLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t>> {
};

TEST_P(GraphReplayLatencyTest, Test) {
    GraphReplayLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.replayCount = std::get<1>(GetParam());
    args.forkCount = std::get<2>(GetParam());

    GraphReplayLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    GraphReplayLatencyTest,
    GraphReplayLatencyTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(5u),
        ::testing::Values(0u, 1u, 2u, 4u, 8u)));
