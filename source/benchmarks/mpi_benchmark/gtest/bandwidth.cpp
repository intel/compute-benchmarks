/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/bandwidth.h"

#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiBandwidth> registerTestCase{};

class MpiBandwidthTest : public ::testing::TestWithParam<std::tuple<uint32_t, UsmMemoryPlacement, UsmMemoryPlacement, uint32_t, MpiStatisticsType>> {};

TEST_P(MpiBandwidthTest, Test) {
    MpiBandwidthArguments args{};

    args.api = Api::L0;
    args.messageSize = std::get<0>(GetParam());
    args.sendType = std::get<1>(GetParam());
    args.recvType = std::get<2>(GetParam());
    args.nBatches = std::get<3>(GetParam());
    args.statsType = std::get<4>(GetParam());

    MpiBandwidth test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiBandwidthTest,
    MpiBandwidthTest,
    ::testing::Combine(
        ::testing::Values(1, 8, 64, 1024, 4096, 65536, 1u << 20),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(200),
        ::testing::ValuesIn(MpiStatisticsTypeArgument::enumValues)));
