/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_concurrency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<QueueConcurrency> registerTestCase{};

class QueueConcurrencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t>> {
};

TEST_P(QueueConcurrencyTest, Test) {
    QueueConcurrencyArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelTime = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam());
    args.kernelCount = std::get<3>(GetParam());

    QueueConcurrency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueueConcurrencyTest,
    QueueConcurrencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1000),
        ::testing::Values(16, 128),
        ::testing::Values(2, 16)));
