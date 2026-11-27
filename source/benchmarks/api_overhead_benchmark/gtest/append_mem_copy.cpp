/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_mem_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include "append_memory_test_names.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMemCopy> registerTestCase{};

using OffsetPair = std::pair<size_t, size_t>;

class AppendMemCopyTest : public ::testing::TestWithParam<std::tuple<size_t, bool, UsmMemoryPlacement, UsmMemoryPlacement, OffsetPair, bool, bool, uint32_t>> {
};

TEST_P(AppendMemCopyTest, Test) {
    const auto [size, useEvent, src, dst, offsets, forceBlitter, useImmediate, appendCount] = GetParam();
    AppendMemCopyArguments args{};
    args.api = Api::L0;
    args.size = size;
    args.useEvent = useEvent;
    args.src = src;
    args.dst = dst;
    args.appendCount = appendCount;
    args.forceBlitter = forceBlitter;
    args.useImmediate = useImmediate;
    args.srcOffset = offsets.first;
    args.dstOffset = offsets.second;

    AppendMemCopy test;
    test.run(args);
}

static std::string appendMemCopyTestName(const ::testing::TestParamInfo<AppendMemCopyTest::ParamType> &info) {
    namespace Names = AppendMemoryTestNames;
    const auto &[size, useEvent, src, dst, offsets, forceBlitter, useImmediate, appendCount] = info.param;
    return Names::size(size) + "_Event" + std::to_string(useEvent) + "_" + Names::placement(src) + "To" + Names::placement(dst) +
           "_SrcOffset" + std::to_string(offsets.first) + "_DstOffset" + std::to_string(offsets.second) + "_" +
           Names::engine(forceBlitter) + "_" + Names::commandList(useImmediate);
}

static const auto placementDeviceHost = ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device);

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyTest,
    AppendMemCopyTest,
    ::testing::Combine(
        ::testing::Values(256 * MemoryConstants::megaByte),
        ::testing::Values(false, true),
        placementDeviceHost,
        placementDeviceHost,
        ::testing::Values(OffsetPair{0, 0}),
        ::testing::Values(false, true),
        ::testing::Values(false),
        ::testing::Values(100)),
    appendMemCopyTestName);

static const auto placementDeviceHostShared = ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared);
static const auto alignedAndUnalignedOffsets = ::testing::Values(OffsetPair{0, 0}, OffsetPair{1, 0}, OffsetPair{1, 1});

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyBigSizeTest,
    AppendMemCopyTest,
    ::testing::Combine(
        ::testing::Values(4 * MemoryConstants::gigaByte, 8 * MemoryConstants::gigaByte, 32 * MemoryConstants::gigaByte),
        ::testing::Values(false),
        placementDeviceHostShared,
        placementDeviceHostShared,
        alignedAndUnalignedOffsets,
        ::testing::Values(false, true),
        ::testing::Values(false),
        ::testing::Values(100)),
    appendMemCopyTestName);

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyBigSizeImmediateTest,
    AppendMemCopyTest,
    ::testing::Combine(
        ::testing::Values(4 * MemoryConstants::gigaByte),
        ::testing::Values(false),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(UsmMemoryPlacement::Device),
        alignedAndUnalignedOffsets,
        ::testing::Values(false, true),
        ::testing::Values(true),
        ::testing::Values(1)),
    appendMemCopyTestName);
