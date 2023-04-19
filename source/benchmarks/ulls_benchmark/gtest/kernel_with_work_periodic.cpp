/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_work_periodic.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelWithWorkPeriodic> registerTestCase{};

class KernelWithWorkPeriodicSubmissionTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t>> {
};

TEST_P(KernelWithWorkPeriodicSubmissionTest, Test) {
    KernelWithWorkPeriodicArguments args;
    args.api = std::get<0>(GetParam());
    args.timeBetweenSubmissions = std::get<1>(GetParam());
    args.numSubmissions = std::get<2>(GetParam());

    KernelWithWorkPeriodic test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithWorkPeriodicSubmissionTest,
    KernelWithWorkPeriodicSubmissionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1'000, 5'000, 50'000),
        ::testing::Values(10)));
