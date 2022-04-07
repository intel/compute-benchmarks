/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/completion_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<CompletionLatency> registerTestCase{};

class CompletionLatencyTest : public ::testing::TestWithParam<Api> {
};

TEST_P(CompletionLatencyTest, Test) {
    CompletionLatencyArguments args{};
    args.api = GetParam();

    CompletionLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CompletionLatencyTest,
    CompletionLatencyTest,
    ::CommonGtestArgs::allApis());
