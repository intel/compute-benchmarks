/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"
[[maybe_unused]] static const inline RegisterTestCase<UsmCopyImmediate> registerTestCase{};

#include <gtest/gtest.h>

class UsmCopyImmediateTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, UsmMemoryPlacement, size_t, BufferContents, bool, bool, bool>> {
};

TEST_P(UsmCopyImmediateTest, Test) {
    UsmCopyImmediateArguments args;
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.destinationPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.contents = std::get<3>(GetParam());
    args.forceBlitter = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());
    args.withCopyOffload = std::get<6>(GetParam());

    if (args.forceBlitter && args.withCopyOffload) {
        GTEST_SKIP(); // If copy offload were to be executed on blitter
    }

    UsmCopyImmediate test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyImmediateTest,
    UsmCopyImmediateTest,
    ::testing::Combine(
        ::testing::ValuesIn(UsmMemoryPlacementArgument::stableTargets),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::limitedTargets),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false, true),
        ::testing::Values(true),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    UsmCopyImmediateTestLIMITED,
    UsmCopyImmediateTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(UsmMemoryPlacement::NonUsm4KBAligned),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(false),
        ::testing::Values(true),
        ::testing::Values(false, true)));
