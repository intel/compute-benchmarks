/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/unmap_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<UnmapBuffer> registerTestCase{};

class UnmapBufferTest : public ::testing::TestWithParam<std::tuple<Api, size_t, BufferContents, bool, MapFlags, bool>> {
};

TEST_P(UnmapBufferTest, Test) {
    UnmapBufferArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.contents = std::get<2>(GetParam());
    args.compressed = std::get<3>(GetParam());
    args.mapFlags = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    UnmapBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UnmapBufferTest,
    UnmapBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128 * megaByte, 512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false, true),
        ::testing::Values(MapFlags::Read, MapFlags::Write, MapFlags::WriteInvalidate),
        ::testing::Values(true)));
