/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/sin_kernel_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SinKernelGraph> registerTestCase{};

class SinKernelGraphTest : public ::testing::TestWithParam<std::tuple<std::size_t, bool>> {
};

TEST_P(SinKernelGraphTest, Test) {
    SinKernelGraphArguments args{};
    args.api = Api::SYCL;
    args.multiplier = std::get<0>(GetParam());
    args.run_with_graph = std::get<1>(GetParam());

    SinKernelGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SinKernelGraphTest,
    SinKernelGraphTest,
    ::testing::Combine(
        ::testing::Values(20, 30, 50, 100, 200),
        ::testing::Values(false, true)));
