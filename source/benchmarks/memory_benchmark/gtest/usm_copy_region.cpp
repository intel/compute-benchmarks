/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_region.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

using Tuple = ThreeComponentUintArgument::TupleType;
static const inline RegisterTestCase<UsmCopyRegion> registerTestCase{};

class UsmCopyRegionTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, Tuple, Tuple, UsmMemoryPlacement, Tuple, Tuple, bool, bool, bool>> {
};

TEST_P(UsmCopyRegionTest, Test) {
    UsmCopyRegionArguments args;
    args.api = Api::L0;
    args.dstptr = std::get<0>(GetParam());
    args.dstRegion = std::get<1>(GetParam());
    args.dstOrigin = std::get<2>(GetParam());
    args.srcptr = std::get<3>(GetParam());
    args.srcRegion = std::get<4>(GetParam());
    args.srcOrigin = std::get<5>(GetParam());
    args.forceBlitter = std::get<6>(GetParam());
    args.useEvents = std::get<7>(GetParam());
    args.reuseCommandList = std::get<8>(GetParam());
    UsmCopyRegion test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyRegionTest,
    UsmCopyRegionTest,
    ::testing::Combine(
        ::testing::ValuesIn(UsmMemoryPlacementArgument::limitedTargets),
        ::testing::Values(Tuple(128, 128, 1), Tuple(128, 128, 128), Tuple(128, 1024, 1024), Tuple(1024, 16, 1024), Tuple(1024, 1024, 16)),
        ::testing::Values(Tuple(0, 0, 0)),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::enumValues),
        ::testing::Values(Tuple(128, 128, 1), Tuple(128, 128, 128), Tuple(128, 1024, 1024), Tuple(1024, 16, 1024), Tuple(1024, 1024, 16)),
        ::testing::Values(Tuple(0, 0, 0)),
        ::testing::Values(false, true),
        ::testing::Values(true),
        ::testing::Values(false, true)));
