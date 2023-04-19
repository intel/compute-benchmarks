/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_entire_image.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include "definitions/map_buffer.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopyEntireImage> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;
class CopyEntireImageTest : public ::testing::TestWithParam<std::tuple<Api, ImageSize, bool, bool>> {
};

TEST_P(CopyEntireImageTest, Test) {
    CopyEntireImageArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.forceBlitter = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    CopyEntireImage test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    CopyEntireImageTest,
    CopyEntireImageTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(
            ImageSize(8192, 1, 1),  // 1D
            ImageSize(16384, 1, 1), // 1D

            ImageSize(256, 512, 1), // 2D
            ImageSize(512, 512, 1), // 2D

            ImageSize(512, 512, 2), // 3D
            ImageSize(512, 512, 64) // 3D
            ),
        ::testing::Values(false),
        ::testing::Values(true)));
