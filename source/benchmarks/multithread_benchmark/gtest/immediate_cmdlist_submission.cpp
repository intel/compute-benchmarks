/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/immediate_cmdlist_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ImmediateCommandListSubmission> registerTestCase{};

class ImmediateCommandListSubmissionLatencyTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t>> {
};

TEST_P(ImmediateCommandListSubmissionLatencyTest, Test) {
    ImmediateCommandListSubmissionArguments args{};
    args.api = Api::L0;
    args.numberOfThreads = std::get<0>(GetParam());
    args.threadsPerEngine = std::get<1>(GetParam());

    ImmediateCommandListSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ImmediateCommandListSubmissionLatencyTest,
    ImmediateCommandListSubmissionLatencyTest,
    testing::Values(
        std::make_tuple(1, 1),
        std::make_tuple(2, 2),
        std::make_tuple(8, 8),
        std::make_tuple(16, 16),
        std::make_tuple(32, 32)));
