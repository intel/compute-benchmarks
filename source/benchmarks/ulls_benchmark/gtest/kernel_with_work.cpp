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

class KernelWithWorkSubmissionTest : public ::testing::TestWithParam<std::tuple<Api, WorkItemIdUsage, size_t, size_t>> {
};

TEST_P(KernelWithWorkSubmissionTest, Test) {
    KernelWithWorkArguments args;
    args.api = std::get<0>(GetParam());
    args.usedIds = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.workgroupSize = std::get<3>(GetParam());

    KernelWithWork test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkSubmissionTest,
    KernelWithWorkSubmissionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(WorkItemIdUsage::None, WorkItemIdUsage::Global, WorkItemIdUsage::Local, WorkItemIdUsage::AtomicPerWorkgroup),
        ::CommonGtestArgs::workgroupCount(),
        ::CommonGtestArgs::workgroupSize()));

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkSubmissionTestLIMITED,
    KernelWithWorkSubmissionTest,
    ::testing::ValuesIn([] {
        std::vector<std::tuple<Api, WorkItemIdUsage, size_t, size_t>> testCases;
        testCases.emplace_back(Api::L0, WorkItemIdUsage::Local, 10000, 32);
        testCases.emplace_back(Api::OpenCL, WorkItemIdUsage::Global, 1000, 256);
        testCases.emplace_back(Api::OpenCL, WorkItemIdUsage::Local, 1000, 256);
        testCases.emplace_back(Api::OpenCL, WorkItemIdUsage::None, 1000, 256);
        return testCases;
    }()));
