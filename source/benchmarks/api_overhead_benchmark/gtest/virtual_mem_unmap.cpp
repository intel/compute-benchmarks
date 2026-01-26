/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_unmap.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<VirtualMemUnMap> registerTestCase{};

class VirtualMemUnMapTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(VirtualMemUnMapTest, Test) {
    VirtualMemUnMapArguments args{};
    args.api = Api::L0;
    args.reserveSize = GetParam();
    VirtualMemUnMap test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemUnMapTest,
    VirtualMemUnMapTest,
    ::testing::Values(2 * megaByte, 64 * megaByte));
