/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_memory_copy_from_context.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMemoryCopyFromContext> registerTestCase{};

class AppendMemoryCopyFromContextTest : public ::testing::TestWithParam<std::tuple<size_t, bool, UsmRuntimeMemoryPlacement, UsmMemoryPlacement, size_t, bool>> {
};

TEST_P(AppendMemoryCopyFromContextTest, Test) {
    AppendMemoryCopyFromContextArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.useEvent = std::get<1>(GetParam());
    args.src = std::get<2>(GetParam());
    args.dst = std::get<3>(GetParam());
    args.appendCount = std::get<4>(GetParam());
    args.forceBlitter = std::get<5>(GetParam());

    AppendMemoryCopyFromContext test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMemoryCopyFromContextTest,
    AppendMemoryCopyFromContextTest,
    ::testing::Combine(
        ::testing::Values(4 * MemoryConstants::kiloByte, 256 * MemoryConstants::megaByte),
        ::testing::Values(false, true),
        ::testing::Values(UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(100),
        ::testing::Values(false, true)));
