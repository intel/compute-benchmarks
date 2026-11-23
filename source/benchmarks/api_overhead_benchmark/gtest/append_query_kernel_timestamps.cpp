/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_query_kernel_timestamps.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendQueryKernelTimestamps> registerTestCase{};

class AppendQueryKernelTimestampsTest : public ::testing::TestWithParam<std::tuple<size_t, bool, UsmMemoryPlacement, bool, size_t>> {
};

TEST_P(AppendQueryKernelTimestampsTest, Test) {
    AppendQueryKernelTimestampsArguments args{};
    args.api = Api::L0;
    args.eventCount = std::get<0>(GetParam());
    args.useOffsets = std::get<1>(GetParam());
    args.dst = std::get<2>(GetParam());
    args.useEvent = std::get<3>(GetParam());
    args.appendCount = std::get<4>(GetParam());

    AppendQueryKernelTimestamps test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendQueryKernelTimestampsTest,
    AppendQueryKernelTimestampsTest,
    ::testing::Combine(
        ::testing::Values(16),
        ::testing::Values(false),
        ::testing::Values(UsmMemoryPlacement::Device),
        ::testing::Values(false),
        ::testing::Values(100)));
