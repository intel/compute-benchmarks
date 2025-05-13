/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/full_remote_access_xe_cores_distributed.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<FullRemoteAccessMemoryXeCoresDistributed> registerTestCase{};

class FullRemoteAccessMemoryXeCoresDistributedTest : public ::testing::TestWithParam<std::tuple<Api, StreamMemoryType, size_t, size_t, bool, size_t, bool>> {
};

TEST_P(FullRemoteAccessMemoryXeCoresDistributedTest, Test) {
    FullRemoteAccessMemoryXeCoresDistributedArguments args;
    args.api = std::get<0>(GetParam());
    args.type = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.elementSize = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.workItems = std::get<5>(GetParam());
    args.blockAccess = std::get<6>(GetParam());

    FullRemoteAccessMemoryXeCoresDistributed test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    FullRemoteAccessMemoryXeCoresDistributedTest,
    FullRemoteAccessMemoryXeCoresDistributedTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::ValuesIn(StreamMemoryTypeArgument::onlyReadAndWrite),
        ::testing::Values(1 * gigaByte),
        ::testing::Values(1, 2, 4, 8),
        ::testing::Values(true),
        ::testing::Values(2048, 4096, 8192, 16384, 32768, 65536),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    FullRemoteAccessMemoryXeCoresDistributedTestLIMITED,
    FullRemoteAccessMemoryXeCoresDistributedTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(StreamMemoryType::Write),
        ::testing::Values(1 * gigaByte),
        ::testing::Values(8),
        ::testing::Values(true),
        ::testing::Values(65536),
        ::testing::Values(false)));
