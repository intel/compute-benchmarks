/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/physical_mem_create.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<PhysicalMemCreate> registerTestCase{};

class PhysicalMemCreateTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(PhysicalMemCreateTest, Test) {
    PhysicalMemCreateArguments args{};
    args.api = Api::L0;
    args.reserveSize = GetParam();
    PhysicalMemCreate test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    PhysicalMemCreateTest,
    PhysicalMemCreateTest,
    ::testing::Values(64 * kiloByte, 1 * gigaByte, 3 * gigaByte));
