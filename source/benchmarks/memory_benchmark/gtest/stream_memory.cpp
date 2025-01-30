/*
 * Copyright (C) 2022-2025 Intel Corporation
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

class StreamMemoryTest : public ::testing::TestWithParam<std::tuple<Api, StreamMemoryType, size_t, bool, BufferContents, UsmMemoryPlacement, size_t>> {
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
        ::testing::Values(1 * megaByte, 8 * megaByte, 32 * megaByte, 128 * megaByte, 512 * megaByte, 1 * gigaByte),
        ::testing::Values(false, true),
        ::testing::Values(BufferContents::Zeros, BufferContents::Random),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::deviceAndHost),
        ::testing::Values(1u)));

INSTANTIATE_TEST_SUITE_P(
    StreamMemoryTestLIMITED,
    StreamMemoryTest,
    ::testing::ValuesIn([] {
        std::vector<std::tuple<Api, StreamMemoryType, size_t, bool, BufferContents, UsmMemoryPlacement, size_t>> testCases;
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Read, 1 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Read, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Read, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Read, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Read, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Scale, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Scale, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Scale, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Scale, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Triad, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Triad, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Triad, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Triad, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Write, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Write, 512 * megaByte, true, BufferContents::Random, UsmMemoryPlacement::Host, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Write, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Device, 1u);
        testCases.emplace_back(Api::OpenCL, StreamMemoryType::Write, 512 * megaByte, true, BufferContents::Zeros, UsmMemoryPlacement::Host, 1u);
        return testCases;
    }()));
