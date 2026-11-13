/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/round_trip_ping_pong.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RoundTripPingPong> registerTestCase{};

class RoundTripPingPongTest : public ::testing::TestWithParam<UsmRuntimeMemoryPlacement> {
};

TEST_P(RoundTripPingPongTest, Test) {
    RoundTripPingPongArguments args{};
    args.api = Api::L0;
    args.placement = GetParam();

    RoundTripPingPong test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    RoundTripPingPongTest,
    RoundTripPingPongTest,
    ::testing::Values(UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Host, UsmRuntimeMemoryPlacement::Shared));
