/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_free.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<VirtualMemFree> registerTestCase{};

class VirtualMemFreeTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(VirtualMemFreeTest, Test) {
    VirtualMemFreeArguments args{};
    args.api = Api::L0;
    args.freeSize = GetParam();
    VirtualMemFree test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemFreeTest,
    VirtualMemFreeTest,
    ::testing::Values(64 * kiloByte, 1 * gigaByte, 3 * gigaByte));
