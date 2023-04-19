/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_submission_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<BestWalkerSubmissionImmediate> registerTestCase{};

class BestWalkerSubmissionImmediateTest : public ::testing::TestWithParam<Api> {
};

TEST_P(BestWalkerSubmissionImmediateTest, Test) {
    BestWalkerSubmissionImmediateArguments args{};
    args.api = GetParam();

    BestWalkerSubmissionImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerSubmissionImmediateTest,
    BestWalkerSubmissionImmediateTest,
    ::testing::Values(Api::L0));
