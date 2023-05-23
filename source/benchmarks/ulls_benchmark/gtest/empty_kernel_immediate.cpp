/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/empty_kernel_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EmptyKernelImmediate> registerTestCase{};

class EmptyKernelSubmissionImmediateTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool>> {
};

TEST_P(EmptyKernelSubmissionImmediateTest, Test) {
    EmptyKernelImmediateArguments args;
    args.api = Api::L0;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useEventForHostSync = std::get<2>(GetParam());

    EmptyKernelImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EmptyKernelSubmissionImmediateTest,
    EmptyKernelSubmissionImmediateTest,
    ::testing::Combine(
        ::CommonGtestArgs::workgroupCount(),
        ::CommonGtestArgs::workgroupSize(),
        ::testing::Values(true, false)));
