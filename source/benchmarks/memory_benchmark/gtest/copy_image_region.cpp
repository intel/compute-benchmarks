/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_image_region.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include "definitions/map_buffer.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopyImageRegion> registerTestCase{};

using Tuple = ThreeComponentUintArgument::TupleType;
class CopyImageRegionTest : public ::testing::TestWithParam<std::tuple<Api, Tuple, bool, bool>> {
};

TEST_P(CopyImageRegionTest, Test) {
    CopyImageRegionArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.forceBlitter = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    CopyImageRegion test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    CopyImageRegionTest,
    CopyImageRegionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(Tuple(128, 128, 1), Tuple(128, 128, 128), Tuple(128, 1024, 1024), Tuple(1024, 16, 1024), Tuple(1024, 1024, 16)),
        ::testing::Values(false),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    CopyImageRegionTestLIMITED,
    CopyImageRegionTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL),
        ::testing::Values(Tuple(1024, 1024, 16)),
        ::testing::Values(false),
        ::testing::Values(true)));
