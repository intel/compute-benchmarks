/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/non_usm_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"
[[maybe_unused]] static const inline RegisterTestCase<NonUsmCopy> registerTestCase{};

#include <gtest/gtest.h>

class NonUsmCopyTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, UsmMemoryPlacement, size_t, bool, bool, bool>> {
};

TEST_P(NonUsmCopyTest, Test) {
    NonUsmCopyArguments args;
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.destinationPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.updateOnHost = std::get<3>(GetParam());
    args.reallocate = std::get<4>(GetParam());
    args.prefetch = std::get<5>(GetParam());

    NonUsmCopy test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    NonUsmCopyTestD2H,
    NonUsmCopyTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::nonUsmTargets),
        ::testing::Values(64 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    NonUsmCopyTestH2D,
    NonUsmCopyTest,
    ::testing::Combine(
        ::testing::ValuesIn(UsmMemoryPlacementArgument::nonUsmTargets),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(64 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    NonUsmCopyTestPrefetchD2H,
    NonUsmCopyTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::nonUsmTargets),
        ::testing::Values(64 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    NonUsmCopyTestPrefetchH2D,
    NonUsmCopyTest,
    ::testing::Combine(
        ::testing::ValuesIn(UsmMemoryPlacementArgument::nonUsmTargets),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(64 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    NonUsmCopyTestLIMITED,
    NonUsmCopyTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(UsmMemoryPlacement::NonUsm4KBAligned),
        ::testing::Values(64 * megaByte),
        ::testing::Values(true),
        ::testing::Values(false, true),
        ::testing::Values(false)));
