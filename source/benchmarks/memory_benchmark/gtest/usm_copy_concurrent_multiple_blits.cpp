/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_concurrent_multiple_blits.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyConcurrentMultipleBlits> registerTestCase{};

class UsmCopyConcurrentMultipleBlitsTest : public ::testing::TestWithParam<std::tuple<
                                               size_t,
                                               std::bitset<maxNumberOfEngines>,
                                               std::bitset<maxNumberOfEngines>>> {
};

TEST_P(UsmCopyConcurrentMultipleBlitsTest, Test) {
    UsmCopyConcurrentMultipleBlitsArguments args;
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.h2dBlitters = std::get<1>(GetParam());
    args.d2hBlitters = std::get<2>(GetParam());

    UsmCopyConcurrentMultipleBlits test;
    test.run(args);
}
using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyConcurrentMultipleBlitsTest,
    UsmCopyConcurrentMultipleBlitsTest,
    ::testing::Values(std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "11111111", "00000000"},
                      std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "00000000", "11111111"},
                      std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "00001110", "11110000"},
                      std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "11110000", "00001110"},
                      std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "00000001", "00000001"},
                      std::tuple<size_t, std::bitset<9>, std::bitset<9>>{256 * megaByte, "10001000", "00100010"}));