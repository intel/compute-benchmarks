/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_priority_preemption.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopyPriorityPreemption> registerTestCase{};

class CopyPriorityPreemptionTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, UsmMemoryPlacement, UsmMemoryPlacement, bool>> {
};

TEST_P(CopyPriorityPreemptionTest, Test) {
    CopyPriorityPreemptionArguments args{};
    args.api = std::get<0>(GetParam());
    args.longCopySize = std::get<1>(GetParam());
    args.shortCopySize = std::get<2>(GetParam());
    args.src = std::get<3>(GetParam());
    args.dst = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());

    CopyPriorityPreemption test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CopyPriorityPreemptionTest,
    CopyPriorityPreemptionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(256 * MemoryConstants::megaByte),
        ::testing::Values(4 * MemoryConstants::kiloByte),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(false, true)));
