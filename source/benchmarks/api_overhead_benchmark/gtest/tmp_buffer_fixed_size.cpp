/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/tmp_buffer_fixed_size.h"

#include "framework/enum/tmp_memory_strategy.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<TmpBufferFixedSize> registerTestCase{};

class TmpBufferFixedSizeTest : public ::testing::TestWithParam<std::tuple<Api, size_t, TmpMemoryStrategy, size_t>> {
};

TEST_P(TmpBufferFixedSizeTest, Test) {
    TmpBufferFixedSizeArguments args{};
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.strategy = std::get<2>(GetParam());
    args.numKernels = std::get<3>(GetParam());
    TmpBufferFixedSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    TmpBufferFixedSizeTest,
    TmpBufferFixedSizeTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(32,
                          64 * MemoryConstants::kiloByte,
                          64 * MemoryConstants::megaByte),
        ::testing::Values(TmpMemoryStrategy::Async, TmpMemoryStrategy::Static, TmpMemoryStrategy::Sync),
        ::testing::Values(10)));

INSTANTIATE_TEST_SUITE_P(
    TmpBufferFixedSizeTestLIMITED,
    TmpBufferFixedSizeTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(4 * MemoryConstants::megaByte),
        ::testing::Values(TmpMemoryStrategy::Async),
        ::testing::Values(10)));
