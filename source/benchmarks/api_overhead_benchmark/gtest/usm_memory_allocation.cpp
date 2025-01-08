/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_memory_allocation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmMemoryAllocation> registerTestCase{};

class UsmMemoryAllocationTest : public ::testing::TestWithParam<std::tuple<Api, UsmRuntimeMemoryPlacement, size_t, AllocationMeasureMode>> {
};

TEST_P(UsmMemoryAllocationTest, Test) {
    UsmMemoryAllocationArguments args{};
    args.api = std::get<0>(GetParam());
    args.usmMemoryPlacement = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.measureMode = std::get<3>(GetParam());
    UsmMemoryAllocation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    UsmMemoryAllocationTest,
    UsmMemoryAllocationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmRuntimeMemoryPlacement::Host, UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Shared),
        ::testing::Values(4,
                          64,
                          512,
                          4 * MemoryConstants::kiloByte,
                          64 * MemoryConstants::kiloByte,
                          512 * MemoryConstants::kiloByte,
                          4 * MemoryConstants::megaByte,
                          64 * MemoryConstants::megaByte,
                          512 * MemoryConstants::megaByte,
                          1 * MemoryConstants::gigaByte),
        ::testing::Values(AllocationMeasureMode::Allocate, AllocationMeasureMode::Free, AllocationMeasureMode::Both)));

INSTANTIATE_TEST_SUITE_P(
    UsmMemoryAllocationTestLIMITED,
    UsmMemoryAllocationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmRuntimeMemoryPlacement::Shared),
        ::testing::Values(
            4 * MemoryConstants::megaByte),
        ::testing::Values(AllocationMeasureMode::Both)));
