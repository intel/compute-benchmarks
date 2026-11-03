/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_image_copy_from_memory_ext.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendImageCopyFromMemoryExt> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;

class AppendImageCopyFromMemoryExtTest : public ::testing::TestWithParam<std::tuple<ImageSize, bool, UsmMemoryPlacement, bool, size_t, bool>> {
};

TEST_P(AppendImageCopyFromMemoryExtTest, Test) {
    AppendImageCopyFromMemoryExtArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.wholeImage = std::get<1>(GetParam());
    args.src = std::get<2>(GetParam());
    args.useEvent = std::get<3>(GetParam());
    args.appendCount = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());

    AppendImageCopyFromMemoryExt test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendImageCopyFromMemoryExtTest,
    AppendImageCopyFromMemoryExtTest,
    ::testing::Combine(
        ::testing::Values(ImageSize(256, 256, 1)),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared),
        ::testing::Values(false, true),
        ::testing::Values(100),
        ::testing::Values(false, true)));
