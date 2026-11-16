/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/image_copy_from_memory.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ImageCopyFromMemory> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;
using ImageTypeAndSize = std::tuple<ImageType, ImageSize>;
class ImageCopyFromMemoryTest : public ::testing::TestWithParam<std::tuple<Api, ImageTypeAndSize, ImageFormatLayout, ImageFormatType, UsmMemoryPlacement, bool, bool>> {
};

TEST_P(ImageCopyFromMemoryTest, Test) {
    ImageCopyFromMemoryArguments args;
    args.api = std::get<0>(GetParam());
    args.imageType = std::get<0>(std::get<1>(GetParam()));
    args.size = std::get<1>(std::get<1>(GetParam()));
    args.layout = std::get<2>(GetParam());
    args.formatType = std::get<3>(GetParam());
    args.sourcePlacement = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());

    ImageCopyFromMemory test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ImageCopyFromMemoryTest,
    ImageCopyFromMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(ImageTypeAndSize(ImageType::Image2D, ImageSize(2048, 2048, 1))),
        ::testing::Values(ImageFormatLayout::Layout8, ImageFormatLayout::Layout8_8_8_8, ImageFormatLayout::Layout16_16_16_16, ImageFormatLayout::Layout32_32_32_32, ImageFormatLayout::Layout10_10_10_2),
        ::testing::Values(ImageFormatType::Uint),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host, UsmMemoryPlacement::NonUsm),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    ImageCopyFromMemoryPlanarTest,
    ImageCopyFromMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(ImageTypeAndSize(ImageType::Image2D, ImageSize(2048, 2048, 1))),
        ::testing::Values(ImageFormatLayout::NV12, ImageFormatLayout::P010),
        ::testing::Values(ImageFormatType::Unorm),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    ImageCopyFromMemoryImageTypesTest,
    ImageCopyFromMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(
            ImageTypeAndSize(ImageType::Image1D_Array, ImageSize(16384, 256, 1)),
            ImageTypeAndSize(ImageType::Image3D, ImageSize(512, 512, 16)),
            ImageTypeAndSize(ImageType::Image2D_Array, ImageSize(512, 512, 16))),
        ::testing::Values(ImageFormatLayout::Layout8_8_8_8),
        ::testing::Values(ImageFormatType::Uint),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(false),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    ImageCopyFromMemoryTestLIMITED,
    ImageCopyFromMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(ImageTypeAndSize(ImageType::Image2D, ImageSize(512, 512, 1))),
        ::testing::Values(ImageFormatLayout::Layout8_8_8_8),
        ::testing::Values(ImageFormatType::Uint),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(false),
        ::testing::Values(true)));
