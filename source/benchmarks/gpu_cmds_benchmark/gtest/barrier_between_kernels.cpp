/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/barrier_between_kernels.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BarrierBetweenKernels> registerTestCase{};

class BarrierBetweenKernelsTest : public ::testing::TestWithParam<std::tuple<size_t, UsmMemoryPlacement, size_t, size_t, size_t>> {
};

TEST_P(BarrierBetweenKernelsTest, Test) {
    BarrierBetweenKernelsArguments args{};
    args.api = Api::L0;

    args.remoteAccess = std::get<0>(GetParam());
    args.bytesToFlush = std::get<3>(GetParam());
    args.onlyReads = std::get<2>(GetParam());
    args.flushedMemory = std::get<1>(GetParam());
    args.barrierCount = std::get<4>(GetParam());

    BarrierBetweenKernels test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BarrierBetweenKernelsTest,
    BarrierBetweenKernelsTest,
    ::testing::Combine(
        ::testing::Values(0, 1),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(0, 1),
        ::testing::Values(4, 8, 16, 32, 64, 256, 1024, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728),
        ::testing::Values(1)));

INSTANTIATE_TEST_SUITE_P(
    BarrierBetweenKernelsTestLIMITED,
    BarrierBetweenKernelsTest,
    ::testing::ValuesIn([] {
        std::vector<std::tuple<size_t, UsmMemoryPlacement, size_t, size_t, size_t>> testCases;
        testCases.emplace_back(1, UsmMemoryPlacement::Device, 0, 16384, 1);
        testCases.emplace_back(1, UsmMemoryPlacement::Host, 0, 16384, 1);
        testCases.emplace_back(1, UsmMemoryPlacement::Device, 1, 4, 10);
        return testCases;
    }()));
