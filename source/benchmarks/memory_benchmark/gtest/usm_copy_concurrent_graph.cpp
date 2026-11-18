/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_concurrent_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"
[[maybe_unused]] static const inline RegisterTestCase<UsmConcurrentCopyGraph> registerTestCase{};

#include <gtest/gtest.h>

class UsmConcurrentCopyGraphTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool>> {
};

TEST_P(UsmConcurrentCopyGraphTest, Test) {
    UsmConcurrentCopyGraphArguments args;
    args.api = Api::L0;
    args.size = std::get<1>(GetParam());
    args.withCopyOffload = std::get<2>(GetParam());

    UsmConcurrentCopyGraph test;
    test.run(args);
}
using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmConcurrentCopyGraphTest,
    UsmConcurrentCopyGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(256 * megaByte),
        ::testing::Values(true, false)));
