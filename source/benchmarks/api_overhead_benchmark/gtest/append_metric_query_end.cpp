/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_metric_query_end.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMetricQueryEnd> registerTestCase{};

class AppendMetricQueryEndTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(AppendMetricQueryEndTest, Test) {
    AppendMetricQueryEndArguments args{};
    args.api = Api::L0;
    args.appendCount = std::get<0>(GetParam());
    args.useEvent = std::get<1>(GetParam());

    AppendMetricQueryEnd test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMetricQueryEndTest,
    AppendMetricQueryEndTest,
    ::testing::Combine(
        ::testing::Values(100),
        ::testing::Values(false, true)));
