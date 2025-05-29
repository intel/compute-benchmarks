/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/tmp_buffer_mixed_size.h"

#include "framework/enum/tmp_memory_strategy.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<TmpBufferMixedSize> registerTestCase{};

class TmpBufferMixedSizeTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, TmpMemoryStrategy, size_t>> {
};

TEST_P(TmpBufferMixedSizeTest, Test) {
    TmpBufferMixedSizeArguments args{};
    args.api = std::get<0>(GetParam());
    args.sizeSmall = std::get<1>(GetParam());
    args.sizeLargeRatio = std::get<2>(GetParam());
    args.strategy = std::get<3>(GetParam());
    args.numKernelsSmall = std::get<4>(GetParam());
    TmpBufferMixedSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    TmpBufferMixedSizeTest,
    TmpBufferMixedSizeTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(32,
                          8 * MemoryConstants::kiloByte,
                          8 * MemoryConstants::megaByte),
        ::testing::Values(4, 16),
        ::testing::Values(TmpMemoryStrategy::Async, TmpMemoryStrategy::Static, TmpMemoryStrategy::Sync),
        ::testing::Values(10)));

INSTANTIATE_TEST_SUITE_P(
    TmpBufferMixedSizeTestLIMITED,
    TmpBufferMixedSizeTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(4 * MemoryConstants::megaByte),
        ::testing::Values(8),
        ::testing::Values(TmpMemoryStrategy::Async),
        ::testing::Values(10)));
