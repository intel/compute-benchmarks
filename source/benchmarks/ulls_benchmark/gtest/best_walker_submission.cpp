/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BestWalkerSubmission> registerTestCase{};

class BestWalkerSubmissionTest : public ::testing::TestWithParam<Api> {
};

TEST_P(BestWalkerSubmissionTest, Test) {
    BestWalkerSubmissionArguments args{};
    args.api = GetParam();

    BestWalkerSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerSubmissionTest,
    BestWalkerSubmissionTest,
    ::CommonGtestArgs::allApis());
