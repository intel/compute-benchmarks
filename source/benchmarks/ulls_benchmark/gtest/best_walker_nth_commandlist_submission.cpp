/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_nth_commandlist_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<BestWalkerNthCommandListSubmission> registerTestCase{};

class BestWalkerNthCommandListSubmissionTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(BestWalkerNthCommandListSubmissionTest, Test) {
    BestWalkerNthCommandListSubmissionArguments args{};
    args.api = Api::L0;
    args.cmdListCount = std::get<0>(GetParam());

    BestWalkerNthCommandListSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerNthCommandListSubmissionTest,
    BestWalkerNthCommandListSubmissionTest,
    ::testing::Values(2));
