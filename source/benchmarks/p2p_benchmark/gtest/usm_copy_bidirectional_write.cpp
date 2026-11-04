/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_bidirectional_write.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>
#include <utility>

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyBidirectionalWrite> registerTestCase{};

class UsmCopyBidirectionalWriteTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, std::pair<Engine, Engine>, bool>> {
};

TEST_P(UsmCopyBidirectionalWriteTest, Test) {
    UsmCopyBidirectionalWriteArguments args;
    args.api = std::get<0>(GetParam());
    args.localDeviceId = std::get<1>(GetParam());
    args.peerDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.localEngine = std::get<4>(GetParam()).first;
    args.peerEngine = std::get<4>(GetParam()).second;
    args.useEvents = std::get<5>(GetParam());

    UsmCopyBidirectionalWrite test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyBidirectionalWriteTest,
    UsmCopyBidirectionalWriteTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0),
        ::testing::Values(1),
        ::testing::Values(16 * megaByte, 256 * megaByte, 1 * gigaByte),
        ::testing::Values(std::make_pair(Engine::Bcs0, Engine::Bcs1)),
        ::testing::Values(true)));
