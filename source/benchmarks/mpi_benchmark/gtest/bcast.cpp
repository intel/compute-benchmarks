/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/bcast.h"

#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiBcast> registerTestCase{};

class MpiBcastTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t, UsmMemoryPlacement, MpiStatisticsType>> {};

TEST_P(MpiBcastTest, Test) {
    MpiBcastArguments args{};

    args.api = Api::L0;
    args.numberOfRanks = std::get<0>(GetParam());
    args.messageSize = std::get<1>(GetParam());
    args.bufferType = std::get<2>(GetParam());
    args.statsType = std::get<3>(GetParam());

    MpiBcast test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiBcastTest,
    MpiBcastTest,
    ::testing::Combine(
        ::testing::Values(2, 4, 8, 16, 32),
        ::testing::Values(1, 8, 64, 1024, 4096, 65536, 1u << 20),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::ValuesIn(MpiStatisticsTypeArgument::enumValues)));
