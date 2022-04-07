/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multi_process_compute_shared_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<MultiProcessComputeSharedBuffer> registerTestCase{};

class MultiProcessComputeSharedBufferTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, size_t, size_t, bool>> {
};

TEST_P(MultiProcessComputeSharedBufferTest, Test) {
    MultiProcessComputeSharedBufferArguments args{};
    args.api = std::get<0>(GetParam());
    args.deviceSelection = std::get<1>(GetParam());
    args.processesPerTile = std::get<2>(GetParam());
    args.workgroupsPerProcess = std::get<3>(GetParam());
    args.synchronize = std::get<4>(GetParam());

    MultiProcessComputeSharedBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    MultiProcessComputeSharedBufferTest,
    MultiProcessComputeSharedBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(
            DeviceSelection::Tile0,
            DeviceSelection::Tile1,
            DeviceSelection::Tile0 | DeviceSelection::Tile1,
            DeviceSelection::Tile0 | DeviceSelection::Tile1 | DeviceSelection::Tile2 | DeviceSelection::Tile3),
        ::testing::Values(1, 2, 4),
        ::testing::Values(1, 300),
        ::testing::Values(false, true)));
