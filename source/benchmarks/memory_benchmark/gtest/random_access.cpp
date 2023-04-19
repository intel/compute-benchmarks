/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/random_access.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RandomAccess> registerTestCase{};

class RandomAccessTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, std::string, size_t>> {
};

TEST_P(RandomAccessTest, Test) {
    RandomAccessArguments args;
    args.api = Api::L0;
    args.allocationSize = std::get<0>(GetParam());
    args.alignment = std::get<1>(GetParam());
    args.accessMode = std::get<2>(GetParam());
    args.randomAccessRange = std::get<3>(GetParam());

    RandomAccess test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    RandomAccessTest,
    RandomAccessTest,
    ::testing::Combine(
        ::testing::Values(256 * megaByte, 1 * gigaByte, 8 * gigaByte, 16 * gigaByte),
        ::testing::Values(64 * kiloByte, 1 * gigaByte),
        ::testing::Values("Read", "Write", "ReadWrite"),
        ::testing::Values(100)));