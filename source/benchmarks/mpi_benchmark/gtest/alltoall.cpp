/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/alltoall.h"

#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiAllToAll> registerTestCase{};

class MpiAllToAllTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t, UsmMemoryPlacement, UsmMemoryPlacement, MpiStatisticsType>> {};

TEST_P(MpiAllToAllTest, Test) {
    MpiAllToAllArguments args{};

    args.api = Api::L0;
    args.numberOfRanks = std::get<0>(GetParam());
    args.messageSize = std::get<1>(GetParam());
    args.sendType = std::get<2>(GetParam());
    args.recvType = std::get<3>(GetParam());
    args.statsType = std::get<4>(GetParam());

    MpiAllToAll test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiAllToAllTest,
    MpiAllToAllTest,
    ::testing::Combine(
        ::testing::Values(2, 4, 8, 16, 32),
        ::testing::Values(1, 8, 64, 1024, 4096, 65536, 1u << 20),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::ValuesIn(MpiStatisticsTypeArgument::enumValues)));
