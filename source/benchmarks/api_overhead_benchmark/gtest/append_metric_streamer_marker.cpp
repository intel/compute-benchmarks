/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_metric_streamer_marker.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMetricStreamerMarker> registerTestCase{};

class AppendMetricStreamerMarkerTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AppendMetricStreamerMarkerTest, Test) {
    AppendMetricStreamerMarkerArguments args{};
    args.api = Api::L0;
    args.appendCount = GetParam();

    AppendMetricStreamerMarker test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMetricStreamerMarkerTest,
    AppendMetricStreamerMarkerTest,
    ::testing::Values(100));
