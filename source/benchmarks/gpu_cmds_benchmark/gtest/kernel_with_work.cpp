/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_work.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelWithWork> registerTestCase{};

class KernelWithWorkTest : public ::testing::TestWithParam<std::tuple<WorkItemIdUsage, size_t, size_t, size_t>> {
};

TEST_P(KernelWithWorkTest, Test) {
    KernelWithWorkArguments args{};
    args.api = Api::L0;
    args.usedIds = std::get<0>(GetParam());
    args.measuredCommands = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.workgroupSize = std::get<3>(GetParam());

    KernelWithWork test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkTest,
    KernelWithWorkTest,
    ::testing::Combine(
        ::testing::Values(WorkItemIdUsage::None, WorkItemIdUsage::Global, WorkItemIdUsage::Local),
        ::testing::Values(500),
        ::CommonGtestArgs::workgroupCount(),
        ::CommonGtestArgs::workgroupSize()));

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkTestLIMITED,
    KernelWithWorkTest,
    ::testing::Combine(
        ::testing::Values(WorkItemIdUsage::None, WorkItemIdUsage::Global, WorkItemIdUsage::Local),
        ::testing::Values(500),
        ::testing::Values(100),
        ::testing::Values(16)));
