/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_nth_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BestWalkerNthSubmission> registerTestCase{};

class BestWalkerNthSubmissionTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(BestWalkerNthSubmissionTest, Test) {
    BestWalkerNthSubmissionArguments args{};
    args.api = Api::L0;
    args.kernelCount = std::get<0>(GetParam());

    BestWalkerNthSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerNthSubmissionTest,
    BestWalkerNthSubmissionTest,
    ::testing::Values(2));
