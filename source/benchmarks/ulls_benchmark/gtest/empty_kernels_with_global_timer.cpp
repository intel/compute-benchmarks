/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/empty_kernels_with_global_timer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EmptyKernelsWithGlobalTimer> registerTestCase{};

class EmptyKernelsWithGlobalTimerTest : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(EmptyKernelsWithGlobalTimerTest, Test) {
    EmptyKernelsWithGlobalTimerArguments args;
    args.api = Api::L0;
    args.kernelCount = std::get<1>(GetParam());

    EmptyKernelsWithGlobalTimer test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EmptyKernelsWithGlobalTimerTest,
    EmptyKernelsWithGlobalTimerTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(16, 32, 64)));

INSTANTIATE_TEST_SUITE_P(
    EmptyKernelsWithGlobalTimerTestLIMITED,
    EmptyKernelsWithGlobalTimerTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(16)));
