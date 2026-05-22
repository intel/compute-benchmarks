/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/sin_kernel_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SinKernelGraph> registerTestCase{};

class SinKernelGraphTest
    : public ::testing::TestWithParam<std::tuple<Api, uint32_t, bool, bool, bool, bool>> {};

TEST_P(SinKernelGraphTest, Test) {
    SinKernelGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());
    args.immediateAppendCmdList = std::get<2>(GetParam());
    args.withCopyOffload = std::get<3>(GetParam());
    args.withGraphs = std::get<4>(GetParam());
    args.useNativeRecording = std::get<5>(GetParam());

    SinKernelGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SinKernelGraphTestSycl, SinKernelGraphTest,
    ::testing::Combine(::testing::Values(Api::SYCL),
                       ::testing::Values(3, 10, 50, 100),
                       // FIXME: immediateAppendCmdList is currently broken, add 'true' to enable it once the driver is fixed.
                       ::testing::Values(false),
                       ::testing::Values(false, true),
                       ::testing::Values(false, true),
                       ::testing::Values(false, true))); // useNativeRecording (SYCL-only)

INSTANTIATE_TEST_SUITE_P(
    SinKernelGraphTestUr, SinKernelGraphTest,
    ::testing::Combine(::testing::Values(Api::UR),
                       ::testing::Values(3, 10, 50, 100),
                       // FIXME: immediateAppendCmdList is currently broken, add 'true' to enable it once the driver is fixed.
                       ::testing::Values(false),
                       ::testing::Values(false, true),
                       ::testing::Values(false, true),
                       ::testing::Values(false))); // useNativeRecording (SYCL-only)

INSTANTIATE_TEST_SUITE_P(
    SinKernelGraphTestL0, SinKernelGraphTest,
    ::testing::Combine(::testing::Values(Api::L0),
                       ::testing::Values(3, 10, 50, 100),
                       // FIXME: immediateAppendCmdList is currently broken, add 'true' to enable it once the driver is fixed.
                       ::testing::Values(false),
                       ::testing::Values(false, true),
                       ::testing::Values(false, true),
                       ::testing::Values(false))); // useNativeRecording (SYCL-only)
