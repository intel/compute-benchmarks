/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/read_image.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ReadImage> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;
class ReadImageTest : public ::testing::TestWithParam<std::tuple<Api, ImageSize, bool, HostptrReuseMode, bool, size_t>> {
};

TEST_P(ReadImageTest, Test) {
    ReadImageArguments args;
    args.api = Api::OpenCL;
    args.size = std::get<1>(GetParam());
    args.forceBlitter = std::get<2>(GetParam());
    args.hostPtrPlacement = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.hostPtrAlignment = std::get<5>(GetParam());

    ReadImage test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    ReadImageTest,
    ReadImageTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(
            ImageSize(8192, 1, 1),  // 1D
            ImageSize(16384, 1, 1), // 1D
            ImageSize(256, 512, 1), // 2D
            ImageSize(512, 512, 1), // 2D
            ImageSize(513, 512, 1), // 2D
            ImageSize(512, 512, 2), // 3D
            ImageSize(512, 512, 64) // 3D
            ),
        ::testing::Values(false),
        ::testing::ValuesIn(HostptrBufferReuseModeArgument::enumValues),
        ::testing::Values(true, false),
        ::testing::Values(1u, 256u)));

INSTANTIATE_TEST_SUITE_P(
    ReadImageTestLIMITED,
    ReadImageTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(
            ImageSize(16384, 1, 1) // 1D
            ),
        ::testing::Values(false),
        ::testing::Values(HostptrReuseMode::Aligned4KB),
        ::testing::Values(true),
        ::testing::Values(1u, 256u)));
