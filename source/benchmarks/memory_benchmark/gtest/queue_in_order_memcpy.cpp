/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_in_order_memcpy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<QueueInOrderMemcpy> registerTestCase{};

class QueueInOrderMemcpyTest : public ::testing::TestWithParam<std::tuple<Api, UsmMemoryPlacement, UsmMemoryPlacement, size_t, size_t, bool, bool>> {};

TEST_P(QueueInOrderMemcpyTest, Test) {
    QueueInOrderMemcpyArguments args{};
    args.api = std::get<0>(GetParam());
    args.sourcePlacement = std::get<1>(GetParam());
    args.destinationPlacement = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.count = std::get<4>(GetParam());
    args.isCopyOnly = std::get<5>(GetParam());
    args.withCopyOffload = std::get<6>(GetParam());

    if (args.isCopyOnly && args.withCopyOffload) {
        GTEST_SKIP(); // If copy offload were to be executed on blitter
    }

    QueueInOrderMemcpy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueueInOrderMemcpyTest,
    QueueInOrderMemcpyTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::L0),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device),
        ::testing::Values(1 * MemoryConstants::megaByte),
        ::testing::Values(10),
        ::testing::Values(true, false),
        ::testing::Values(true, false)));
