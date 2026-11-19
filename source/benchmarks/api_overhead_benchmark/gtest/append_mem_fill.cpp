/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_mem_fill.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMemFill> registerTestCase{};

class AppendMemFillTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, UsmMemoryPlacement, size_t, bool, bool>> {
};

TEST_P(AppendMemFillTest, Test) {
    const auto [size, patternSize, dst, dstOffset, forceBlitter, useImmediate] = GetParam();
    AppendMemFillArguments args{};
    args.api = Api::L0;
    args.size = size;
    args.patternSize = patternSize;
    args.useEvent = false;
    args.dst = dst;
    args.dstOffset = dstOffset;
    args.appendCount = useImmediate ? 1 : 100;
    args.forceBlitter = forceBlitter;
    args.useImmediate = useImmediate;

    AppendMemFill test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 32 * MemoryConstants::gigaByte, 64 * MemoryConstants::gigaByte),
        ::testing::Values(1, 4),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared),
        ::testing::Values(0, 1),
        ::testing::Values(false, true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillImmediateDeviceTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 32 * MemoryConstants::gigaByte, 64 * MemoryConstants::gigaByte),
        ::testing::Values(1, 4),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(0, 1),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillImmediateHostSharedTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(MemoryConstants::megaByte, 4 * MemoryConstants::gigaByte),
        ::testing::Values(1, 4),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared),
        ::testing::Values(0, 1),
        ::testing::Values(false, true),
        ::testing::Values(true)));
