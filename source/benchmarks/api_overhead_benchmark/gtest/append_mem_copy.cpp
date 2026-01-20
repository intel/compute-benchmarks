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

    AppendMemCopy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemCopyTest,
    AppendMemCopyTest,
    ::testing::Combine(
        ::testing::Values(256 * MemoryConstants::megaByte, 4 * MemoryConstants::gigaByte),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(100),
        ::testing::Values(false, true)));
