/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_metric_memory_barrier.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMetricMemoryBarrier> registerTestCase{};

class AppendMetricMemoryBarrierTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AppendMetricMemoryBarrierTest, Test) {
    AppendMetricMemoryBarrierArguments args{};
    args.api = Api::L0;
    args.appendCount = GetParam();

    AppendMetricMemoryBarrier test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMetricMemoryBarrierTest,
    AppendMetricMemoryBarrierTest,
    ::testing::Values(100));
