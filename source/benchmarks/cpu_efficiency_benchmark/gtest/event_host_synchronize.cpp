/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_host_synchronize.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>
#include <tuple>

[[maybe_unused]] static const inline RegisterTestCase<EventHostSynchronize> registerTestCase{};

class EventHostSynchronizeTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool, bool>> {
};

TEST_P(EventHostSynchronizeTest, Test) {
    EventHostSynchronizeArguments args{};
    args.api = Api::L0;
    args.kernelExecutionTime = std::get<0>(GetParam());
    args.batchSize = std::get<1>(GetParam());
    args.useKernelTimestamps = std::get<2>(GetParam());
    args.inOrderQueue = std::get<3>(GetParam());

    EventHostSynchronize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventHostSynchronizeTest,
    EventHostSynchronizeTest,
    ::testing::Values(
        std::make_tuple(100u, 1000u, false, false),
        std::make_tuple(10000u, 100u, false, false),
        std::make_tuple(50000u, 20u, false, false),
        std::make_tuple(150000u, 5u, false, false),
        std::make_tuple(100u, 1000u, true, true),
        std::make_tuple(10000u, 100u, true, true),
        std::make_tuple(50000u, 20u, true, true),
        std::make_tuple(150000u, 5u, true, true)));
