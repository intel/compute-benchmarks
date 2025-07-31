/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_barrier.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitBarrier> registerTestCase{};

class SubmitBarrierTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, bool, bool, bool>> {
};

TEST_P(SubmitBarrierTest, Test) {
    SubmitBarrierArguments args{};
    args.api = std::get<0>(GetParam());
    args.useHostTasks = std::get<1>(GetParam());
    args.inOrderQueue = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    args.submitBarrier = std::get<4>(GetParam());
    args.getLastEvent = std::get<5>(GetParam());
    SubmitBarrier test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SubmitBarrierTest,
    SubmitBarrierTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false, true),   // useHostTasks
        ::testing::Values(false, true),   // inOrderQueue
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(false, true),   // submitBarrier
        ::testing::Values(false, true))); // getLastEvent