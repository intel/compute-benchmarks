/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/overlap.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MpiOverlap> registerTestCase{};

class MpiOverlapTest : public ::testing::TestWithParam<std::tuple<uint32_t, UsmMemoryPlacement, UsmMemoryPlacement, bool, uint32_t>> {};

TEST_P(MpiOverlapTest, Test) {
    MpiOverlapArguments args{};

    args.api = Api::L0;
    args.messageSize = std::get<0>(GetParam());
    args.sendType = std::get<1>(GetParam());
    args.recvType = std::get<2>(GetParam());
    args.gpuCompute = std::get<3>(GetParam());
    args.numMpiTestCalls = std::get<4>(GetParam());

    MpiOverlap test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MpiOverlapTest,
    MpiOverlapTest,
    ::testing::Combine(
        ::testing::Values(1u << 8, 1u << 14, 1u << 20),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Bool(),
        ::testing::Values(0, 2)));
