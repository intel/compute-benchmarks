/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/enqueue_barrier_with_empty_waitlist.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<EnqueueBarrierWithEmptyWaitlist> registerTestCase{};

class EnqueueBarrierWithEmptyWaitlistTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(EnqueueBarrierWithEmptyWaitlistTest, Test) {
    EnqueueBarrierWithEmptyWaitlistArguments args{};
    args.api = Api::OpenCL;
    args.enqueueCount = std::get<0>(GetParam());
    args.outOfOrderQueue = std::get<1>(GetParam());

    EnqueueBarrierWithEmptyWaitlist test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EnqueueBarrierWithEmptyWaitlistTest,
    EnqueueBarrierWithEmptyWaitlistTest,
    ::testing::Combine(
        ::testing::Values(16, 32, 64),
        ::testing::Bool()));
