/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_nth_submission_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BestWalkerNthSubmissionImmediate> registerTestCase{};

class BestWalkerNthSubmissionImmediateTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(BestWalkerNthSubmissionImmediateTest, Test) {
    BestWalkerNthSubmissionImmediateArguments args{};
    args.api = Api::L0;
    args.kernelCount = std::get<0>(GetParam());

    BestWalkerNthSubmissionImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerNthSubmissionImmediateTest,
    BestWalkerNthSubmissionImmediateTest,
    ::testing::Values(2));
