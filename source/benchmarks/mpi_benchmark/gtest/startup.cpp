/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/startup.h"

#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiStartup> registerTestCase{};

class MpiStartupTest : public ::testing::TestWithParam<std::tuple<uint32_t, bool, int32_t, MpiStatisticsType>> {};

TEST_P(MpiStartupTest, Test) {
    MpiStartupArguments args{};

    args.api = Api::L0;
    args.numberOfRanks = std::get<0>(GetParam());
    args.initMpiFirst = std::get<1>(GetParam());
    args.initFlag = std::get<2>(GetParam());
    args.statsType = std::get<3>(GetParam());

    MpiStartup test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiStartupTest,
    MpiStartupTest,
    ::testing::Combine(
        ::testing::Values(1, 2, 4, 8, 16, 32),
        ::testing::Bool(),
        ::testing::Values(0, 1, 2),
        ::testing::ValuesIn(MpiStatisticsTypeArgument::enumValues)));
