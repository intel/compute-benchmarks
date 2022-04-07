/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<WriteBuffer> registerTestCase{};

class WriteBufferTest : public ::testing::TestWithParam<std::tuple<Api, size_t, BufferContents, bool, bool, HostptrReuseMode>> {
};

TEST_P(WriteBufferTest, Test) {
    WriteBufferArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.contents = std::get<2>(GetParam());
    args.compressed = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.reuse = std::get<5>(GetParam());

    WriteBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    WriteBufferTest,
    WriteBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128 * megaByte, 512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::ValuesIn(HostptrBufferReuseModeArgument::enumValues)));
