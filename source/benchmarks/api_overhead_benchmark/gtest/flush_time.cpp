/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/flush_time.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<FlushTime> registerTestCase{};

class FlushTimeTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, bool, size_t>> {
};

TEST_P(FlushTimeTest, Test) {
    FlushTimeArguments args{};
    args.api = Api::OpenCL;
    args.workgroupCount = std::get<0>(GetParam());
    args.workgroupSize = std::get<1>(GetParam());
    args.useOoq = std::get<2>(GetParam());
    args.useEvent = std::get<3>(GetParam());
    args.flushCount = std::get<4>(GetParam());

    FlushTime test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    FlushTimeTest,
    FlushTimeTest,
    ::testing::Combine(
        ::testing::Values(1, 100, 1000),
        ::testing::Values(0, 1, 32, 256),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(100)));
