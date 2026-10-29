/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/z_multi_queue_graph_submission.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ZMultiQueueGraphSubmission> registerTestCase{};

class ZMultiQueueGraphSubmissionTest : public ::testing::TestWithParam<std::tuple<CcsBitmaskArgument::Bitset, BcsBitmaskArgument::Bitset, bool>> {
};

TEST_P(ZMultiQueueGraphSubmissionTest, Test) {
    ZMultiQueueGraphSubmissionArguments args{};
    args.api = Api::L0;
    args.ccsMask = std::get<0>(GetParam());
    args.bcsMask = std::get<1>(GetParam());
    args.useHpCopyEngine = std::get<2>(GetParam());

    ZMultiQueueGraphSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ZMultiQueueGraphSubmissionTest,
    ZMultiQueueGraphSubmissionTest,
    ::testing::Combine(
        ::testing::Values("0000", "0001"),
        ::testing::Values("000000001", "011111111"),
        ::testing::Values(false, true)));
