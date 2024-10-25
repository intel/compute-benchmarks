/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_image.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WriteImage> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;
class WriteImageTest : public ::testing::TestWithParam<std::tuple<Api, ImageSize, bool, HostptrReuseMode, bool>> {
};

TEST_P(WriteImageTest, Test) {
    WriteImageArguments args;
    args.api = Api::OpenCL;
    args.size = std::get<1>(GetParam());
    args.forceBlitter = std::get<2>(GetParam());
    args.hostPtrPlacement = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());

    WriteImage test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    WriteImageTest,
    WriteImageTest,
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
        ::testing::ValuesIn(HostptrBufferReuseModeArgument::enumValues),
        ::testing::Values(true, false)));
