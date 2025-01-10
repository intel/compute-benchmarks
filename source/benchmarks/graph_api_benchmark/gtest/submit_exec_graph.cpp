/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_exec_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitExecGraph> registerTestCase{};

class SubmitExecGraphTest : public ::testing::TestWithParam<std::tuple<bool, std::size_t, bool>> {
};

TEST_P(SubmitExecGraphTest, Test) {
    SubmitExecGraphArguments args{};
    args.api = Api::SYCL;
    args.measureSubmit = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());
    args.ioq = std::get<2>(GetParam());

    SubmitExecGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SubmitExecGraphTest,
    SubmitExecGraphTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(50, 100, 500),
        ::testing::Values(false, true)));
