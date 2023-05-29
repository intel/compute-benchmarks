/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/full_remote_access.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<FullRemoteAccessMemory> registerTestCase{};

class FullRemoteAccessMemoryTest : public ::testing::TestWithParam<std::tuple<Api, StreamMemoryType, size_t, size_t, bool, size_t, bool>> {
};

TEST_P(FullRemoteAccessMemoryTest, Test) {
    FullRemoteAccessMemoryArguments args;
    args.api = std::get<0>(GetParam());
    args.type = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.elementSize = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.workItems = std::get<5>(GetParam());
    args.blockAccess = std::get<6>(GetParam());

    FullRemoteAccessMemory test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    FullRemoteAccessMemoryTest,
    FullRemoteAccessMemoryTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::ValuesIn(StreamMemoryTypeArgument::onlyReadAndWrite),
        ::testing::Values(1 * gigaByte),
        ::testing::Values(1, 2, 4, 8),
        ::testing::Values(true),
        ::testing::Values(1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072),
        ::testing::Values(false, true)));
