/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/image_recreation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ImageRecreation> registerTestCase{};

using ImageSize = ThreeComponentUintArgument::TupleType;

class ImageRecreationTest : public ::testing::TestWithParam<std::tuple<Api, ImageSize, bool>> {
};

TEST_P(ImageRecreationTest, Test) {
    ImageRecreationArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.writeWholeImage = std::get<2>(GetParam());

    ImageRecreation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ImageRecreationTest,
    ImageRecreationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(
            ImageSize(256, 256, 1),
            ImageSize(1920, 1080, 1),
            ImageSize(3840, 2160, 1)),
        ::testing::Values(false, true)));
