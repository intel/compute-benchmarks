/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WriteLatency> registerTestCase{};

class WriteLatencyTest : public ::testing::TestWithParam<Api> {
};

TEST_P(WriteLatencyTest, Test) {
    WriteLatencyArguments args{};
    args.api = GetParam();

    WriteLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WriteLatencyTest,
    WriteLatencyTest,
    ::CommonGtestArgs::allApis());
