/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/non_usm_stream_memory.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<NonUsmStreamMemory> registerTestCase{};

class NonUsmStreamMemoryTest : public ::testing::TestWithParam<std::tuple<StreamMemoryType, size_t, bool, BufferContents, UsmMemoryPlacement, size_t, size_t, size_t, bool>> {
};

TEST_P(NonUsmStreamMemoryTest, Test) {
    NonUsmStreamMemoryArguments args;
    args.api = Api::L0;
    args.type = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.useEvents = std::get<2>(GetParam());
    args.contents = std::get<3>(GetParam());
    args.memoryPlacement = std::get<4>(GetParam());
    args.partialMultiplier = std::get<5>(GetParam());
    args.vectorSize = std::get<6>(GetParam());
    args.lws = std::get<7>(GetParam());
    args.prefetch = std::get<8>(GetParam());

    NonUsmStreamMemory test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTest,
    NonUsmStreamMemoryTest,
    ::testing::Combine(
        ::testing::ValuesIn(StreamMemoryTypeArgument::enumValues),
        ::testing::Values(1 * megaByte, 8 * megaByte, 32 * megaByte, 128 * megaByte, 512 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::deviceAndHost),
        ::testing::Values(1u),
        ::testing::Values(1),
        ::testing::Values(1024),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTestNonUsm,
    NonUsmStreamMemoryTest,
    ::testing::Combine(
        ::testing::ValuesIn(StreamMemoryTypeArgument::enumValues),
        ::testing::Values(1 * megaByte, 8 * megaByte, 32 * megaByte, 256 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::nonUsmTargets),
        ::testing::Values(1u),
        ::testing::Values(1),
        ::testing::Values(1024),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTestLIMITED,
    NonUsmStreamMemoryTest,
    ::testing::Combine(
        ::testing::Values(StreamMemoryType::Triad),
        ::testing::Values(512 * megaByte),
        ::testing::Values(true),
        ::testing::Values(BufferContents::Random),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(1u),
        ::testing::Values(4),
        ::testing::Values(1024, 256),
        ::testing::Values(false)));
