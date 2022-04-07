/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/event_ctxt_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<EventCtxtSwitchLatency> registerTestCase{};

class EventCtxtSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<size_t, Engine, Engine>> {
};

TEST_P(EventCtxtSwitchLatencyTest, Test) {
    EventCtxtSwitchLatencyArguments args{};
    args.api = Api::L0;
    args.measuredCommands = std::get<0>(GetParam());
    args.firstEngine = std::get<1>(GetParam());
    args.secondEngine = std::get<2>(GetParam());

    EventCtxtSwitchLatency test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    EventCtxtSwitchLatencyTest,
    EventCtxtSwitchLatencyTest,
    ::testing::Combine(
        ::testing::Values(500, 1000),
        ::testing::Values(Engine::Ccs0),
        ::testing::Values(Engine::Bcs)));
