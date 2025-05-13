/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_batch_memory_allocation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmBatchMemoryAllocation> registerTestCase{};

class UsmBatchMemoryAllocationTest : public ::testing::TestWithParam<std::tuple<Api, UsmRuntimeMemoryPlacement, size_t, size_t, AllocationMeasureMode>> {
};

TEST_P(UsmBatchMemoryAllocationTest, Test) {
    UsmBatchMemoryAllocationArguments args{};
    args.api = std::get<0>(GetParam());
    args.usmMemoryPlacement = std::get<1>(GetParam());
    args.allocationCount = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.measureMode = std::get<4>(GetParam());
    UsmBatchMemoryAllocation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    UsmBatchMemoryAllocationTest,
    UsmBatchMemoryAllocationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmRuntimeMemoryPlacement::Host, UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Shared),
        ::testing::Values(32, 512),
        ::testing::Values(4 * MemoryConstants::kiloByte,
                          64 * MemoryConstants::kiloByte,
                          512 * MemoryConstants::kiloByte,
                          4 * MemoryConstants::megaByte),
        ::testing::Values(AllocationMeasureMode::Allocate, AllocationMeasureMode::Free, AllocationMeasureMode::Both)));
