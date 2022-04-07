/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<BestSubmission> registerTestCase{};

class BestSubmissionTest : public ::testing::TestWithParam<Api> {
};

TEST_P(BestSubmissionTest, Test) {
    BestSubmissionArguments args{};
    args.api = GetParam();

    BestSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestSubmissionTest,
    BestSubmissionTest,
    ::CommonGtestArgs::allApis());
