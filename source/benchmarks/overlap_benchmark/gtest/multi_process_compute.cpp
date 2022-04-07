/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_process_compute.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<MultiProcessCompute> registerTestCase{};

class MultiProcessComputeTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, size_t, size_t, bool, size_t>> {
};

TEST_P(MultiProcessComputeTest, Test) {
    MultiProcessComputeArguments args{};
    args.api = std::get<0>(GetParam());
    args.deviceSelection = std::get<1>(GetParam());
    args.processesPerTile = std::get<2>(GetParam());
    args.workgroupsPerProcess = std::get<3>(GetParam());
    args.synchronize = std::get<4>(GetParam());
    args.operationsPerKernelCount = std::get<5>(GetParam());
    MultiProcessCompute test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    MultiProcessComputeTest,
    MultiProcessComputeTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(
            DeviceSelection::Tile0,
            DeviceSelection::Tile1,
            DeviceSelection::Tile0 | DeviceSelection::Tile1,
            DeviceSelection::Tile0 | DeviceSelection::Tile1 | DeviceSelection::Tile2 | DeviceSelection::Tile3),
        ::testing::Values(1, 2, 4, 8),
        ::testing::Values(1, 300),
        ::testing::Values(false, true),
        ::testing::Values(5000, 500000)));
