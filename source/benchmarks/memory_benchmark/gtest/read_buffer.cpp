/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/read_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ReadBuffer> registerTestCase{};

class ReadBufferTest : public ::testing::TestWithParam<std::tuple<Api, size_t, BufferContents, bool, bool, HostptrReuseMode>> {
};

TEST_P(ReadBufferTest, Test) {
    ReadBufferArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.contents = std::get<2>(GetParam());
    args.compressed = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.reuse = std::get<5>(GetParam());

    ReadBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    ReadBufferTest,
    ReadBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128 * megaByte, 512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::ValuesIn(HostptrBufferReuseModeArgument::enumValues)));

INSTANTIATE_TEST_SUITE_P(
    ReadBufferTestLIMITED,
    ReadBufferTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false),
        ::testing::Values(true),
        ::testing::Values(HostptrReuseMode::Aligned4KB)));
