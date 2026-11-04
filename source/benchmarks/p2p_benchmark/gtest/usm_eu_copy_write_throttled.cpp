/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_eu_copy_write_throttled.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmEuCopyWriteThrottled> registerTestCase{};

class UsmEuCopyWriteThrottledTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, Engine, size_t, bool>> {
};

TEST_P(UsmEuCopyWriteThrottledTest, Test) {
    UsmEuCopyWriteThrottledArguments args;
    args.api = std::get<0>(GetParam());
    args.localDeviceId = std::get<1>(GetParam());
    args.peerDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.engine = std::get<4>(GetParam());
    args.throttledWorkItems = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());

    UsmEuCopyWriteThrottled test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmEuCopyWriteThrottledTest,
    UsmEuCopyWriteThrottledTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(256 * megaByte, 1 * gigaByte),
        ::testing::Values(Engine::Ccs0),
        ::testing::Values(2816),
        ::testing::Values(true)));
