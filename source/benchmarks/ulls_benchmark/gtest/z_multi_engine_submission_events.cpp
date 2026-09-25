/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/z_multi_engine_submission_events.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ZMultiEngineSubmissionEvents> registerTestCase{};

class ZMultiEngineSubmissionEventsTest : public ::testing::TestWithParam<std::tuple<CcsBitmaskArgument::Bitset, BcsBitmaskArgument::Bitset, bool>> {
};

TEST_P(ZMultiEngineSubmissionEventsTest, Test) {
    ZMultiEngineSubmissionEventsArguments args{};
    args.api = Api::L0;
    args.ccsMask = std::get<0>(GetParam());
    args.bcsMask = std::get<1>(GetParam());
    args.useHpCopyEngine = std::get<2>(GetParam());

    ZMultiEngineSubmissionEvents test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ZMultiEngineSubmissionEventsTest,
    ZMultiEngineSubmissionEventsTest,
    ::testing::Combine(
        ::testing::Values("1111"),
        ::testing::Values("111111111"),
        ::testing::Values(true)));
