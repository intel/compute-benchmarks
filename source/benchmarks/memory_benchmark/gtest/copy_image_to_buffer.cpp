/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_image_to_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include "definitions/map_buffer.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopyImageToBuffer> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;
class CopyImageToBufferTest : public ::testing::TestWithParam<std::tuple<Api, UsmMemoryPlacement, size_t, ImageSize, bool, bool>> {
};

TEST_P(CopyImageToBufferTest, Test) {
    CopyImageToBufferArguments args;
    args.api = std::get<0>(GetParam());
    args.destinationPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.region = std::get<3>(GetParam());
    args.forceBlitter = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    CopyImageToBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    CopyImageToBufferTest,
    CopyImageToBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(512 * megaByte),
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

INSTANTIATE_TEST_SUITE_P(
    CopyImageToBufferTestLIMITED,
    CopyImageToBufferTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(512 * megaByte),
        ::testing::Values(
            ImageSize(16384, 1, 1) // 1D
            ),
        ::testing::Values(false),
        ::testing::Values(true)));
