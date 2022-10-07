/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/best_walker_submission_immediate_multi_cmdlists.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<BestWalkerSubmissionImmediateMultiCmdlists> registerTestCase{};

class BestWalkerSubmissionImmediateMultiCmdlistsTest : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(BestWalkerSubmissionImmediateMultiCmdlistsTest, Test) {
    BestWalkerSubmissionImmediateMultiCmdlistsArguments args{};
    args.api = std::get<0>(GetParam());
    args.cmdlistCount = std::get<1>(GetParam());

    BestWalkerSubmissionImmediateMultiCmdlists test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    BestWalkerSubmissionImmediateMultiCmdlistsTest,
    BestWalkerSubmissionImmediateMultiCmdlistsTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(2, 4, 8)));
