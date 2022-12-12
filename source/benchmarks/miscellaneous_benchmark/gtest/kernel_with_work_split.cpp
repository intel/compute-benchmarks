/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_work_split.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<KernelWithWorkSplit> registerTestCase{};

class KernelWithWorkSplitSubmissionTest : public ::testing::TestWithParam<std::tuple<Api, WorkItemIdUsage, size_t, size_t, size_t, bool>> {
};

TEST_P(KernelWithWorkSplitSubmissionTest, Test) {
    KernelWithWorkArgumentsSplit args;
    args.api = std::get<0>(GetParam());
    args.usedIds = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.workgroupSize = std::get<3>(GetParam());
    args.splitSize = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    KernelWithWorkSplit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkSplitSubmissionTest,
    KernelWithWorkSplitSubmissionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(WorkItemIdUsage::None, WorkItemIdUsage::Global, WorkItemIdUsage::Local),
        ::testing::Values(128, 1024, 8096),
        ::CommonGtestArgs::workgroupSize(),
        ::testing::Values(1u, 2u, 4u, 8u, 16u, 32u, 64u),
        ::testing::Values(true)));
