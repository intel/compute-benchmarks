/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/slm_switch_latency.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<SlmSwitchLatency> registerTestCase{};

class SlmSwitchLatencyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t>> {
};

TEST_P(SlmSwitchLatencyTest, Test) {
    SlmSwitchLatencyArguments args;
    args.api = std::get<0>(GetParam());
    args.slmPerWkgKernel1 = std::get<1>(GetParam());
    args.slmPerWkgKernel2 = std::get<2>(GetParam());
    args.wgs = std::get<3>(GetParam());

    SlmSwitchLatency test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    SlmSwitchLatencyTest,
    SlmSwitchLatencyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1 * kiloByte, 16 * kiloByte, 32 * kiloByte, 64 * kiloByte),
        ::testing::Values(1 * kiloByte, 16 * kiloByte, 32 * kiloByte, 64 * kiloByte),
        ::testing::Values(128, 256)));
