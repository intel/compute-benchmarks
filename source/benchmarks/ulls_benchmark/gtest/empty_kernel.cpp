/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/empty_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<EmptyKernel> registerTestCase{};

class EmptyKernelSubmissionTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t>> {
};

TEST_P(EmptyKernelSubmissionTest, Test) {
    EmptyKernelArguments args;
    args.api = std::get<0>(GetParam());
    args.workgroupCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());

    EmptyKernel test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EmptyKernelSubmissionTest,
    EmptyKernelSubmissionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::CommonGtestArgs::workgroupCount(),
        ::CommonGtestArgs::workgroupSize()));
