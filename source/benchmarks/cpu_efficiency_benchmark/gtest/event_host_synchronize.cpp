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

class EventHostSynchronizeTest : public ::testing::TestWithParam<std::tuple<size_t, size_t>> {
};

TEST_P(EventHostSynchronizeTest, Test) {
    EventHostSynchronizeArguments args{};
    args.api = Api::L0;
    args.kernelExecutionTime = std::get<0>(GetParam());
    args.batchSize = std::get<1>(GetParam());

    EventHostSynchronize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventHostSynchronizeTest,
    EventHostSynchronizeTest,
    ::testing::Values(
        std::make_tuple(100u, 1000u),
        std::make_tuple(10000u, 100u),
        std::make_tuple(50000u, 20u),
        std::make_tuple(150000u, 5u)));
