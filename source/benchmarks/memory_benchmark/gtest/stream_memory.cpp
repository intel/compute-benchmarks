/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/stream_memory.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<StreamMemory> registerTestCase{};

class StreamMemoryTest : public ::testing::TestWithParam<std::tuple<Api, StreamMemoryType, size_t, bool, BufferContents, UsmRuntimeMemoryPlacement, size_t, size_t, size_t>> {
};

TEST_P(StreamMemoryTest, Test) {
    StreamMemoryArguments args;
    args.api = std::get<0>(GetParam());
    args.type = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    args.contents = std::get<4>(GetParam());
    args.memoryPlacement = std::get<5>(GetParam());
    args.partialMultiplier = std::get<6>(GetParam());
    args.vectorSize = std::get<7>(GetParam());
    args.lws = std::get<8>(GetParam());

    StreamMemory test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTest,
    StreamMemoryTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::ValuesIn(StreamMemoryTypeArgument::enumValues),
        ::testing::Values(1 * megaByte, 8 * megaByte, 32 * megaByte, 128 * megaByte, 512 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::ValuesIn(UsmRuntimeMemoryPlacementArgument::deviceAndHost),
        ::testing::Values(1u),
        ::testing::Values(1),
        ::testing::Values(1024)));

INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTestLIMITED,
    StreamMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL),
        ::testing::Values(StreamMemoryType::Triad),
        ::testing::Values(512 * megaByte),
        ::testing::Values(true),
        ::testing::Values(BufferContents::Random),
        ::testing::Values(UsmRuntimeMemoryPlacement::Device),
        ::testing::Values(1u),
        ::testing::Values(4),
        ::testing::Values(1024, 256)));
