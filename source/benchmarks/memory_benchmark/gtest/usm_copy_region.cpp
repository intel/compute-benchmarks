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
static const inline RegisterTestCase<UsmCopyRegion> registerTestCase{};

#include <gtest/gtest.h>

using Tuple = ThreeComponentUintArgument::TupleType;

class UsmCopyRegionTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, UsmMemoryPlacement, Tuple, Tuple, size_t, BufferContents, bool, bool>> {
};

TEST_P(UsmCopyRegionTest, Test) {
    UsmCopyRegionArguments args;
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.destinationPlacement = std::get<1>(GetParam());
    args.region = std::get<2>(GetParam());
    args.origin = std::get<3>(GetParam());
    args.size = std::get<4>(GetParam());
    args.contents = std::get<5>(GetParam());
    args.forceBlitter = std::get<6>(GetParam());
    args.useEvents = std::get<7>(GetParam());

    UsmCopyRegion test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyRegionTest,
    UsmCopyRegionTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(Tuple(128, 128, 1), Tuple(128, 128, 128), Tuple(128, 1024, 1024), Tuple(1024, 16, 1024), Tuple(1024, 1024, 16)),
        ::testing::Values(Tuple(0, 0, 0)),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false, true),
        ::testing::Values(true)));
