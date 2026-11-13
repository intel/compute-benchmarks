/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_read_multiple_blits.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <bitset>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyReadMultipleBlits> registerTestCase{};

class UsmCopyReadMultipleBlitsTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, std::bitset<9>, bool>> {
};

TEST_P(UsmCopyReadMultipleBlitsTest, Test) {
    UsmCopyReadMultipleBlitsArguments args;
    args.api = std::get<0>(GetParam());
    args.localDeviceId = std::get<1>(GetParam());
    args.peerDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.blitters = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    UsmCopyReadMultipleBlits test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyReadMultipleBlitsTest,
    UsmCopyReadMultipleBlitsTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(256 * megaByte),
        ::testing::Values("000000011"),
        ::testing::Values(true)));
