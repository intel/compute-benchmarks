/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/queue_memcpy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<QueueMemcpy> registerTestCase{};

class QueueMemcpyTest : public ::testing::TestWithParam<std::tuple<UsmMemoryPlacement, UsmMemoryPlacement, size_t>> {};

TEST_P(QueueMemcpyTest, Test) {
    QueueMemcpyArguments args{};
    args.api = Api::SYCL;
    args.sourcePlacement = std::get<0>(GetParam());
    args.destinationPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    QueueMemcpy test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    QueueMemcpyTest,
    QueueMemcpyTest,
    ::testing::Combine(
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device),
        ::testing::Values(UsmMemoryPlacement::Host, UsmMemoryPlacement::Shared, UsmMemoryPlacement::Device),
        ::testing::Values(128 * MemoryConstants::megaByte,
                          512 * MemoryConstants::megaByte)));
