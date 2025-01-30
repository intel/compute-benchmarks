/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/set_kernel_arg_svm_pointer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SetKernelArgSvmPointer> registerTestCase{};

class SetKernelArgSvmPointerTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, size_t>> {
};

TEST_P(SetKernelArgSvmPointerTest, Test) {
    SetKernelArgSvmPointerArguments args{};
    args.api = std::get<0>(GetParam());
    args.allocationsCount = std::get<1>(GetParam());
    args.reallocate = std::get<2>(GetParam());
    args.allocationSize = std::get<3>(GetParam());

    SetKernelArgSvmPointer test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SetKernelArgSvmPointerTest,
    SetKernelArgSvmPointerTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(100),
        ::testing::Values(false, true),
        ::testing::Values(1024)));

INSTANTIATE_TEST_SUITE_P(
    SetKernelArgSvmPointerTestLIMITED,
    SetKernelArgSvmPointerTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL),
        ::testing::Values(100),
        ::testing::Values(true),
        ::testing::Values(1024)));
