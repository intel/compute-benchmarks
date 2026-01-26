/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/immediate_cmdlist_submission.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MultiProcessImmediateCmdlistSubmission> registerTestCase{};

class MultiProcessImmediateCmdlistSubmissionLatencyTest : public ::testing::TestWithParam<std::tuple<uint32_t>> {
};

TEST_P(MultiProcessImmediateCmdlistSubmissionLatencyTest, Test) {
    MultiProcessImmediateCmdlistSubmissionArguments args{};
    args.api = Api::L0;
    args.numberOfProcesses = std::get<0>(GetParam());

    MultiProcessImmediateCmdlistSubmission test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultiProcessImmediateCmdlistSubmissionLatencyTest,
    MultiProcessImmediateCmdlistSubmissionLatencyTest,
    testing::Values(
        std::make_tuple(1),
        std::make_tuple(2),
        std::make_tuple(4),
        std::make_tuple(8),
        std::make_tuple(16)));