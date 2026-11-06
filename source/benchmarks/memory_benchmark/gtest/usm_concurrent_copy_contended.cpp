/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_concurrent_copy_contended.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmConcurrentCopyContended> registerTestCase{};

class UsmConcurrentCopyContendedTest : public ::testing::TestWithParam<std::tuple<Api, size_t, CopyDirection, Engine, Engine, bool>> {
};

TEST_P(UsmConcurrentCopyContendedTest, Test) {
    UsmConcurrentCopyContendedArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.measuredDirection = std::get<2>(GetParam());
    args.h2dEngine = std::get<3>(GetParam());
    args.d2hEngine = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    UsmConcurrentCopyContended test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmConcurrentCopyContendedTest,
    UsmConcurrentCopyContendedTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(256 * megaByte),
        ::testing::Values(CopyDirection::HostToDevice, CopyDirection::DeviceToHost),
        ::testing::Values(Engine::Ccs0),
        ::testing::Values(Engine::Bcs0),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    UsmConcurrentCopyContendedSwappedEnginesTest,
    UsmConcurrentCopyContendedTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(256 * megaByte),
        ::testing::Values(CopyDirection::HostToDevice, CopyDirection::DeviceToHost),
        ::testing::Values(Engine::Bcs0),
        ::testing::Values(Engine::Ccs0),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    UsmConcurrentCopyContendedCopyEnginesTest,
    UsmConcurrentCopyContendedTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(256 * megaByte),
        ::testing::Values(CopyDirection::HostToDevice, CopyDirection::DeviceToHost),
        ::testing::Values(Engine::Bcs1),
        ::testing::Values(Engine::Bcs0),
        ::testing::Values(true)));
