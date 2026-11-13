/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_bidirectional_read_multiple_blits.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <bitset>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyBidirectionalReadMultipleBlits> registerTestCase{};

class UsmCopyBidirectionalReadMultipleBlitsTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, std::tuple<std::bitset<9>, std::bitset<9>>, bool>> {
};

TEST_P(UsmCopyBidirectionalReadMultipleBlitsTest, Test) {
    UsmCopyBidirectionalReadMultipleBlitsArguments args;
    args.api = std::get<0>(GetParam());
    args.localDeviceId = std::get<1>(GetParam());
    args.peerDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.localBlitters = std::get<0>(std::get<4>(GetParam()));
    args.peerBlitters = std::get<1>(std::get<4>(GetParam()));
    args.useEvents = std::get<5>(GetParam());

    UsmCopyBidirectionalReadMultipleBlits test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyBidirectionalReadMultipleBlitsTest,
    UsmCopyBidirectionalReadMultipleBlitsTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(256 * megaByte),
        ::testing::Values(std::make_tuple(std::bitset<9>("000000011"), std::bitset<9>("000000011"))),
        ::testing::Values(true)));
