/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_write.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyWrite> registerTestCase{};

class UsmCopyWriteTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, Engine, bool>> {
};

TEST_P(UsmCopyWriteTest, Test) {
    UsmCopyWriteArguments args;
    args.api = std::get<0>(GetParam());
    args.localDeviceId = std::get<1>(GetParam());
    args.peerDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.engine = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    UsmCopyWrite test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyWriteTest,
    UsmCopyWriteTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(16 * megaByte, 256 * megaByte, 1 * gigaByte),
        ::testing::Values(Engine::Bcs0, Engine::Ccs0),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    UsmCopyWriteTestLIMITED,
    UsmCopyWriteTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(256 * megaByte),
        ::testing::Values(Engine::Bcs0),
        ::testing::Values(true)));
