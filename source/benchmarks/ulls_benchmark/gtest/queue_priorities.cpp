/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_priorities.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<QueuePriorities> registerTestCase{};

class QueuePrioritiesTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, size_t, size_t, size_t>> {
};

TEST_P(QueuePrioritiesTest, Test) {
    QueuePrioritiesArguments args{};
    args.api = std::get<0>(GetParam());
    args.lowPriorityKernelTime = std::get<1>(GetParam());
    args.usePriorities = std::get<2>(GetParam());
    args.workgroupCount = std::get<3>(GetParam());
    args.highPriorityKernelTime = std::get<4>(GetParam());
    args.sleepTime = std::get<5>(GetParam());

    QueuePriorities test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueuePrioritiesTest,
    QueuePrioritiesTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 10),
        ::testing::Values(false, true),
        ::testing::Values(1, 2048),
        ::testing::Values(1, 10),
        ::testing::Values(1, 6)));
