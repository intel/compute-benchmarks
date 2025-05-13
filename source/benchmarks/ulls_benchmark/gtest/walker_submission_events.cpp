/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/walker_submission_events.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<WalkerSubmissionEvents> registerTestCase{};

class WalkerSubmissionEventsTest : public ::testing::TestWithParam<Api> {
};

TEST_P(WalkerSubmissionEventsTest, Test) {
    WalkerSubmissionEventsArguments args{};
    args.api = GetParam();

    WalkerSubmissionEvents test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WalkerSubmissionEventsTest,
    WalkerSubmissionEventsTest,
    ::CommonGtestArgs::allApis());

INSTANTIATE_TEST_SUITE_P(
    WalkerSubmissionEventsTestLIMITED,
    WalkerSubmissionEventsTest,
    ::testing::Values(Api::L0, Api::OpenCL));
