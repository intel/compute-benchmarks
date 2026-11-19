/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_mem_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMemCopy> registerTestCase{};

class AppendMemCopyTest : public ::testing::TestWithParam<std::tuple<size_t, bool, UsmMemoryPlacement, UsmMemoryPlacement, uint32_t, bool>> {
};

TEST_P(AppendMemCopyTest, Test) {
    AppendMemCopyArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.useEvent = std::get<1>(GetParam());
    args.src = std::get<2>(GetParam());
    args.dst = std::get<3>(GetParam());
    args.appendCount = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());
    args.useImmediate = false;
    args.srcOffset = 0;
    args.dstOffset = 0;

    AppendMemCopy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyTest,
    AppendMemCopyTest,
    ::testing::Combine(
        ::testing::Values(256 * MemoryConstants::megaByte),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(100),
        ::testing::Values(false, true)));

using PlacementPair = std::pair<UsmMemoryPlacement, UsmMemoryPlacement>;
using OffsetPair = std::pair<size_t, size_t>;

class AppendMemCopyBigSizeTest : public ::testing::TestWithParam<std::tuple<size_t, PlacementPair, OffsetPair, bool, bool>> {
};

TEST_P(AppendMemCopyBigSizeTest, Test) {
    const auto [size, placements, offsets, forceBlitter, useImmediate] = GetParam();
    AppendMemCopyArguments args{};
    args.api = Api::L0;
    args.size = size;
    args.useEvent = false;
    args.src = placements.first;
    args.dst = placements.second;
    args.appendCount = useImmediate ? 1 : 100;
    args.forceBlitter = forceBlitter;
    args.useImmediate = useImmediate;
    args.srcOffset = offsets.first;
    args.dstOffset = offsets.second;

    AppendMemCopy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyBigSizeTest,
    AppendMemCopyBigSizeTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 4 * MemoryConstants::gigaByte, 8 * MemoryConstants::gigaByte, 32 * MemoryConstants::gigaByte),
        ::testing::Values(PlacementPair{UsmMemoryPlacement::Host, UsmMemoryPlacement::Device},
                          PlacementPair{UsmMemoryPlacement::Device, UsmMemoryPlacement::Host},
                          PlacementPair{UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device},
                          PlacementPair{UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared},
                          PlacementPair{UsmMemoryPlacement::Shared, UsmMemoryPlacement::Shared}),
        ::testing::Values(OffsetPair{0, 0}, OffsetPair{1, 1}),
        ::testing::Values(false, true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyBigSizeImmediateTest,
    AppendMemCopyBigSizeTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 4 * MemoryConstants::gigaByte),
        ::testing::Values(PlacementPair{UsmMemoryPlacement::Host, UsmMemoryPlacement::Device},
                          PlacementPair{UsmMemoryPlacement::Device, UsmMemoryPlacement::Host},
                          PlacementPair{UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device},
                          PlacementPair{UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared},
                          PlacementPair{UsmMemoryPlacement::Shared, UsmMemoryPlacement::Shared}),
        ::testing::Values(OffsetPair{0, 0}, OffsetPair{1, 1}),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyBigSizeDeviceToDeviceTest,
    AppendMemCopyBigSizeTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 4 * MemoryConstants::gigaByte, 8 * MemoryConstants::gigaByte, 32 * MemoryConstants::gigaByte),
        ::testing::Values(PlacementPair{UsmMemoryPlacement::Device, UsmMemoryPlacement::Device}),
        ::testing::Values(OffsetPair{0, 0}, OffsetPair{1, 1}, OffsetPair{1, 0}),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
