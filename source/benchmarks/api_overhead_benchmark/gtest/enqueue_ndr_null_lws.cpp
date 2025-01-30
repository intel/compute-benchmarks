/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/enqueue_ndr_null_lws.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EnqueueNdrNullLws> registerTestCase{};

class EnqueueNdrNullLwsTest : public ::testing::TestWithParam<std::tuple<size_t, bool, bool, bool>> {
};

TEST_P(EnqueueNdrNullLwsTest, Test) {
    EnqueueNdrNullLwsArguments args{};
    args.api = Api::OpenCL;
    args.gws = std::get<0>(GetParam());
    args.useOoq = std::get<1>(GetParam());
    args.useProfiling = std::get<2>(GetParam());
    args.useEvent = std::get<3>(GetParam());

    EnqueueNdrNullLws test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EnqueueNdrNullLwsTest,
    EnqueueNdrNullLwsTest,
    ::testing::Combine(
        ::testing::Values(1, 100, 1000),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    EnqueueNdrNullLwsTestLIMITED,
    EnqueueNdrNullLwsTest,
    ::testing::Combine(
        ::testing::Values(1000),
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(true)));
