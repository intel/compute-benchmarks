/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_metric_query_begin.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMetricQueryBegin> registerTestCase{};

class AppendMetricQueryBeginTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AppendMetricQueryBeginTest, Test) {
    AppendMetricQueryArguments args{};
    args.api = Api::L0;
    args.appendCount = GetParam();

    AppendMetricQueryBegin test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMetricQueryBeginTest,
    AppendMetricQueryBeginTest,
    ::testing::Values(100));
