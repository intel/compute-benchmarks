/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/latency.h"

#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiLatency> registerTestCase{};

class MpiLatencyTest : public ::testing::TestWithParam<std::tuple<uint32_t, UsmMemoryPlacement, UsmMemoryPlacement, MpiStatisticsType>> {};

TEST_P(MpiLatencyTest, Test) {
    MpiLatencyArguments args{};

    args.api = Api::L0;
    args.messageSize = std::get<0>(GetParam());
    args.sendType = std::get<1>(GetParam());
    args.recvType = std::get<2>(GetParam());
    args.statsType = std::get<3>(GetParam());

    MpiLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiLatencyTest,
    MpiLatencyTest,
    ::testing::Combine(
        ::testing::Values(1, 8, 64, 1024, 4096, 65536, 1u << 20),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::ValuesIn(MpiStatisticsTypeArgument::enumValues)));
