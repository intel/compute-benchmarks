/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/host_mapped_memory.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<HostMappedMemory> registerTestCase{};

class HostMappedMemoryTest : public ::testing::TestWithParam<std::tuple<Api, StreamMemoryType, size_t, bool>> {
};

TEST_P(HostMappedMemoryTest, Test) {
    HostMappedMemoryArguments args;
    args.api = std::get<0>(GetParam());
    args.type = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.fenceWrites = std::get<3>(GetParam());

    HostMappedMemory test;
    test.run(args);
}

using namespace MemoryConstants;

INSTANTIATE_TEST_SUITE_P(
    HostMappedMemoryWriteTest,
    HostMappedMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(StreamMemoryType::Write),
        ::testing::Values(256 * megaByte),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    HostMappedMemoryReadTest,
    HostMappedMemoryTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(StreamMemoryType::Read),
        ::testing::Values(1 * megaByte, 8 * megaByte),
        ::testing::Values(false)));
