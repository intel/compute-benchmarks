/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_submission_events.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopySubmissionEvents> registerTestCase{};

class CopySubmissionEventsTest : public ::testing::TestWithParam<std::tuple<Api, Engine>> {
};

TEST_P(CopySubmissionEventsTest, Test) {
    CopySubmissionEventsArguments args{};
    args.api = std::get<0>(GetParam());
    args.engine = std::get<1>(GetParam());

    CopySubmissionEvents test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CopySubmissionEventsTest,
    CopySubmissionEventsTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(Engine::Ccs0, Engine::Ccs1, Engine::Ccs2, Engine::Ccs3, Engine::Bcs, Engine::Bcs1, Engine::Bcs2, Engine::Bcs3, Engine::Bcs4, Engine::Bcs5, Engine::Bcs6, Engine::Bcs7, Engine::Bcs8)));
