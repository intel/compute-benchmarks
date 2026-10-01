/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_eu_reduce_copy_ring.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmEUReduceCopyRing> registerTestCase{};

class UsmEUReduceCopyRingTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, bool>> {
};

TEST_P(UsmEUReduceCopyRingTest, Test) {
    UsmEUReduceCopyRingArguments args;
    args.api = std::get<0>(GetParam());
    args.numDevices = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    UsmEUReduceCopyRing test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmEUReduceCopyRingTest,
    UsmEUReduceCopyRingTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(2, 4, 8),
        ::testing::Values(64 * kiloByte, 128 * kiloByte, 256 * kiloByte, 512 * kiloByte,
                          1 * megaByte, 2 * megaByte, 4 * megaByte, 8 * megaByte, 16 * megaByte, 32 * megaByte,
                          64 * megaByte, 128 * megaByte, 256 * megaByte, 512 * megaByte, 1 * gigaByte),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    UsmEUReduceCopyRingTestLIMITED,
    UsmEUReduceCopyRingTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(2),
        ::testing::Values(16 * megaByte),
        ::testing::Values(false, true)));
