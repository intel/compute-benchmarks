/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_work_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelWithWorkImmediate> registerTestCase{};

class KernelWithWorkSubmissionImmediateTest : public ::testing::TestWithParam<std::tuple<WorkItemIdUsage, size_t, size_t, bool>> {
};

TEST_P(KernelWithWorkSubmissionImmediateTest, Test) {
    KernelWithWorkImmediateArguments args;
    args.api = Api::L0;
    args.usedIds = std::get<0>(GetParam());
    args.workgroupCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.useEventForHostSync = std::get<3>(GetParam());

    KernelWithWorkImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkSubmissionImmediateTest,
    KernelWithWorkSubmissionImmediateTest,
    ::testing::Combine(
        ::testing::Values(WorkItemIdUsage::None, WorkItemIdUsage::Global, WorkItemIdUsage::Local, WorkItemIdUsage::AtomicPerWorkgroup),
        ::CommonGtestArgs::workgroupCount(),
        ::CommonGtestArgs::workgroupSize(),
        ::testing::Values(true, false)));
