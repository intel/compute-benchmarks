/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_mem_fill.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include "append_memory_test_names.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMemFill> registerTestCase{};

class AppendMemFillTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, UsmMemoryPlacement, size_t, bool, bool, uint32_t>> {
};

TEST_P(AppendMemFillTest, Test) {
    const auto [size, patternSize, useEvent, dst, dstOffsetInPatterns, forceBlitter, useImmediate, appendCount] = GetParam();
    AppendMemFillArguments args{};
    args.api = Api::L0;
    args.size = size;
    args.patternSize = patternSize;
    args.useEvent = useEvent;
    args.dst = dst;
    args.dstOffset = dstOffsetInPatterns * patternSize;
    args.appendCount = appendCount;
    args.forceBlitter = forceBlitter;
    args.useImmediate = useImmediate;

    AppendMemFill test;
    test.run(args);
}

static std::string appendMemFillTestName(const ::testing::TestParamInfo<AppendMemFillTest::ParamType> &info) {
    namespace Names = AppendMemoryTestNames;
    const auto &[size, patternSize, useEvent, dst, dstOffsetInPatterns, forceBlitter, useImmediate, appendCount] = info.param;
    return Names::size(size) + "_Pattern" + std::to_string(patternSize) + "_Event" + std::to_string(useEvent) + "_" +
           Names::placement(dst) + "_DstOffset" + std::to_string(dstOffsetInPatterns * patternSize) + "_" +
           Names::engine(forceBlitter) + "_" + Names::commandList(useImmediate);
}

static const auto fillPatternSizes = ::testing::Values(1, 4);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(256 * MemoryConstants::megaByte),
        fillPatternSizes,
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(0),
        ::testing::Values(false, true),
        ::testing::Values(false),
        ::testing::Values(100)),
    appendMemFillTestName);

static const auto fillBigSizes = ::testing::Values(32 * MemoryConstants::gigaByte, 64 * MemoryConstants::gigaByte);
static const auto dstOffsetsInPatterns = ::testing::Values(0, 1);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillBigSizeTest,
    AppendMemFillTest,
    ::testing::Combine(
        fillBigSizes,
        fillPatternSizes,
        ::testing::Values(false),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared),
        dstOffsetsInPatterns,
        ::testing::Values(false, true),
        ::testing::Values(false),
        ::testing::Values(100)),
    appendMemFillTestName);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillBigSizeImmediateDeviceTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(4 * MemoryConstants::gigaByte),
        fillPatternSizes,
        ::testing::Values(false),
        ::testing::Values(UsmMemoryPlacement::Device),
        dstOffsetsInPatterns,
        ::testing::Values(false, true),
        ::testing::Values(true),
        ::testing::Values(1)),
    appendMemFillTestName);
