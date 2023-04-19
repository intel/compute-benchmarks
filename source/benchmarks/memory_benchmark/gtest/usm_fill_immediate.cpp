/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_fill_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmFillImmediate> registerTestCase{};

class UsmFillImmediateTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, size_t, BufferContents, size_t, BufferContents, bool, bool>> {
};

TEST_P(UsmFillImmediateTest, Test) {
    UsmFillImmediateArguments args;
    args.api = Api::L0;
    args.usmMemoryPlacement = std::get<0>(GetParam());
    args.bufferSize = std::get<1>(GetParam());
    args.contents = std::get<2>(GetParam());
    args.patternSize = std::get<3>(GetParam());
    args.patternContents = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());

    UsmFillImmediate test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmFillImmediateTest,
    UsmFillImmediateTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Zeros),
        ::testing::Values(1, 4, 16),
        ::testing::Values(BufferContents::Random),
        ::testing::Values(false, true),
        ::testing::Values(true)));
