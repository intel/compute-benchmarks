/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/exec_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ExecGraph> registerTestCase{};

class ExecGraphTest : public ::testing::TestWithParam<std::tuple<std::size_t>> {
};

TEST_P(ExecGraphTest, Test) {
    ExecGraphArguments args{};
    args.api = Api::SYCL;
    args.numKernels = std::get<0>(GetParam());

    ExecGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ExecGraphTest,
    ExecGraphTest,
    ::testing::Values(50, 100, 500));
