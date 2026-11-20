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

class AppendMemFillTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, UsmMemoryPlacement, uint32_t, bool>> {
};

TEST_P(AppendMemFillTest, Test) {
    AppendMemFillArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.patternSize = std::get<1>(GetParam());
    args.useEvent = std::get<2>(GetParam());
    args.dst = std::get<3>(GetParam());
    args.appendCount = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());
    args.useImmediate = false;
    args.dstOffset = 0;

    AppendMemFill test;
    test.run(args);
}

static std::string appendMemFillTestName(const ::testing::TestParamInfo<AppendMemFillTest::ParamType> &info) {
    namespace Names = AppendMemoryTestNames;
    const auto &[size, patternSize, useEvent, dst, appendCount, forceBlitter] = info.param;
    return Names::size(size) + "_Pattern" + std::to_string(patternSize) + "_Event" + std::to_string(useEvent) + "_" +
           Names::placement(dst) + "_AppendCount" + std::to_string(appendCount) + "_" + Names::engine(forceBlitter);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillTest,
    AppendMemFillTest,
    ::testing::Combine(
        ::testing::Values(256 * MemoryConstants::megaByte),
        ::testing::Values(1, 4),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(100),
        ::testing::Values(false, true)),
    appendMemFillTestName);

class AppendMemFillBigSizeTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, UsmMemoryPlacement, size_t, bool, bool>> {
};

TEST_P(AppendMemFillBigSizeTest, Test) {
    const auto [size, patternSize, dst, dstOffsetInPatterns, forceBlitter, useImmediate] = GetParam();
    AppendMemFillArguments args{};
    args.api = Api::L0;
    args.size = size;
    args.patternSize = patternSize;
    args.useEvent = false;
    args.dst = dst;
    args.dstOffset = dstOffsetInPatterns * patternSize;
    args.appendCount = useImmediate ? 1 : 100;
    args.forceBlitter = forceBlitter;
    args.useImmediate = useImmediate;

    AppendMemFill test;
    test.run(args);
}

static std::string appendMemFillBigSizeTestName(const ::testing::TestParamInfo<AppendMemFillBigSizeTest::ParamType> &info) {
    namespace Names = AppendMemoryTestNames;
    const auto &[size, patternSize, dst, dstOffsetInPatterns, forceBlitter, useImmediate] = info.param;
    return Names::size(size) + "_Pattern" + std::to_string(patternSize) + "_" + Names::placement(dst) +
           "_DstOffset" + std::to_string(dstOffsetInPatterns * patternSize) + "_" + Names::engine(forceBlitter) + "_" +
           Names::commandList(useImmediate);
}

static const auto fillBigSizes = ::testing::Values(32 * MemoryConstants::gigaByte, 64 * MemoryConstants::gigaByte);
static const auto fillPatternSizes = ::testing::Values(1, 4);
static const auto dstOffsetsInPatterns = ::testing::Values(0, 1);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillBigSizeTest,
    AppendMemFillBigSizeTest,
    ::testing::Combine(
        fillBigSizes,
        fillPatternSizes,
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared),
        dstOffsetsInPatterns,
        ::testing::Values(false, true),
        ::testing::Values(false)),
    appendMemFillBigSizeTestName);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillBigSizeImmediateDeviceTest,
    AppendMemFillBigSizeTest,
    ::testing::Combine(
        fillBigSizes,
        fillPatternSizes,
        ::testing::Values(UsmMemoryPlacement::Device),
        dstOffsetsInPatterns,
        ::testing::Values(false, true),
        ::testing::Values(true)),
    appendMemFillBigSizeTestName);

INSTANTIATE_TEST_SUITE_P(
    AppendMemFillBigSizeImmediateHostSharedTest,
    AppendMemFillBigSizeTest,
    ::testing::Combine(
        ::testing::Values(4 * MemoryConstants::gigaByte),
        fillPatternSizes,
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared),
        dstOffsetsInPatterns,
        ::testing::Values(false, true),
        ::testing::Values(true)),
    appendMemFillBigSizeTestName);
