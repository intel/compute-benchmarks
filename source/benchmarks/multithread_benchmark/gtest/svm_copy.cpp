/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/svm_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SvmCopy> registerTestCase{};

class SvmCopyTest : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(SvmCopyTest, Test) {
    SvmCopyArguments args{};
    args.api = std::get<0>(GetParam());
    args.numberOfThreads = std::get<1>(GetParam());

    SvmCopy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SvmCopyTest,
    SvmCopyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 2, 4, 8, 16)));
