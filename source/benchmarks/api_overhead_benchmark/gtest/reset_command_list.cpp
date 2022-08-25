/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/reset_command_list.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ResetCommandList> registerTestCase{};

class ResetCommandListTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, size_t, bool>> {
};

TEST_P(ResetCommandListTest, Test) {
    ResetCommandListArguments args{};
    args.api = Api::L0;
    args.sourcePlacement = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.copyOnly = std::get<2>(GetParam());
    ResetCommandList test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ResetCommandListTest,
    ResetCommandListTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Device, UsmMemoryPlacement::Shared, UsmMemoryPlacement::NonUsm),
        ::testing::Values(4,
                          1 * MemoryConstants::megaByte,
                          128 * MemoryConstants::megaByte),
        ::testing::Values(false, true)));
