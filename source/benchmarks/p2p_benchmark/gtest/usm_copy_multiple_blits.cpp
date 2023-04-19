/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_multiple_blits.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyMultipleBlits> registerTestCase{};

class UsmCopyMultipleBlitsTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, std::bitset<9>>> {
};

TEST_P(UsmCopyMultipleBlitsTest, Test) {
    UsmP2PCopyMultipleBlitsArguments args;
    args.api = std::get<0>(GetParam());
    args.srcDeviceId = std::get<1>(GetParam());
    args.dstDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.blitters = std::get<4>(GetParam());

    if (args.srcDeviceId == args.dstDeviceId) {
        GTEST_SKIP();
    }

    UsmCopyMultipleBlits test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyMultipleBlitsTest,
    UsmCopyMultipleBlitsTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(0, 1),
        ::testing::Values(0, 1),
        ::testing::Values(512 * megaByte),
        ::testing::Values("000000001", "000000010", "000000110", "111111111")));
