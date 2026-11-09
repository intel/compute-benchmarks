/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_concurrent_pairs_read.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyConcurrentPairsRead> registerTestCase{};

class UsmCopyConcurrentPairsReadTest : public ::testing::TestWithParam<std::tuple<Api, std::string, size_t, std::string, bool>> {
};

TEST_P(UsmCopyConcurrentPairsReadTest, Test) {
    UsmCopyConcurrentPairsReadArguments args;
    args.api = std::get<0>(GetParam());
    args.pairs = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.engines = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());

    UsmCopyConcurrentPairsRead test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyConcurrentPairsReadTest,
    UsmCopyConcurrentPairsReadTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values("1:0_2:0_3:0", "0:1_2:3", "0:1_1:2_2:3_3:0", "0:1_1:0_2:3_3:2"),
        ::testing::Values(256 * megaByte),
        ::testing::Values("BCS0", "BCS1"),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    UsmCopyConcurrentPairsReadWithMultipleEnginesTest,
    UsmCopyConcurrentPairsReadTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values("0:1_0:2"),
        ::testing::Values(256 * megaByte),
        ::testing::Values("BCS0_BCS1"),
        ::testing::Values(true)));
