/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_kernels_async.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitKernelsAsync> registerTestCase{};

class SubmitKernelsAsyncTest : public ::testing::TestWithParam<std::tuple<std::size_t, bool>> {
};

TEST_P(SubmitKernelsAsyncTest, Test) {
    SubmitKernelsAsyncArguments args{};
    args.api = Api::L0;
    args.numKernels = std::get<0>(GetParam());
    args.useEventsForSync = std::get<1>(GetParam());

    SubmitKernelsAsync test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SubmitKernelsAsyncTest,
    SubmitKernelsAsyncTest,
    ::testing::Combine(
        ::testing::Values(1, 2, 10, 100),
        ::testing::Values(false, true)));
