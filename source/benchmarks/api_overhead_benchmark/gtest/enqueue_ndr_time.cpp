/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/enqueue_ndr_time.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<EnqueueNdrTime> registerTestCase{};

class EnqueueNdrTimeTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, bool, bool>> {
};

TEST_P(EnqueueNdrTimeTest, Test) {
    EnqueueNdrTimeArguments args{};
    args.api = Api::OpenCL;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useOoq = std::get<2>(GetParam());
    args.useProfiling = std::get<3>(GetParam());
    args.useEvent = std::get<4>(GetParam());

    EnqueueNdrTime test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EnqueueNdrTimeTest,
    EnqueueNdrTimeTest,
    ::testing::Combine(
        ::testing::Values(1, 100, 1000),
        ::testing::Values(1, 32, 256),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
