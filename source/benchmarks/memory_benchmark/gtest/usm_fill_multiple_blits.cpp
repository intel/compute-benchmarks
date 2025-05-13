/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_fill_multiple_blits.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmFillMultipleBlits> registerTestCase{};

class UsmFillMultipleBlitsTest : public ::testing::TestWithParam<std::tuple<Api, UsmMemoryPlacement, size_t, size_t, BufferContents, std::bitset<9>>> {
};

TEST_P(UsmFillMultipleBlitsTest, Test) {
    UsmFillMultipleBlitsArguments args;
    args.api = std::get<0>(GetParam());
    args.memoryPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.patternSize = std::get<3>(GetParam());
    args.patternContents = std::get<4>(GetParam());
    args.blitters = std::get<5>(GetParam());

    UsmFillMultipleBlits test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmFillMultipleBlitsTest,
    UsmFillMultipleBlitsTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(512 * megaByte),
        ::testing::Values(1),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::Values("000000001", "000000010", "000000111", "111111111")));

INSTANTIATE_TEST_SUITE_P(
    UsmFillMultipleBlitsTestLIMITED,
    UsmFillMultipleBlitsTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(512 * megaByte),
        ::testing::Values(1),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::Values("000000111")));
