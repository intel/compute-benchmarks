/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_random_memory_allocation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmRandomMemoryAllocation> registerTestCase{};

class UsmRandomMemoryAllocationTest : public ::testing::TestWithParam<std::tuple<Api, UsmRuntimeMemoryPlacement, size_t, size_t, size_t, DistributionKind>> {
};

TEST_P(UsmRandomMemoryAllocationTest, Test) {
    UsmRandomMemoryAllocationArguments args{};
    args.api = std::get<0>(GetParam());
    args.usmMemoryPlacement = std::get<1>(GetParam());
    args.operationCount = std::get<2>(GetParam());
    args.minSize = std::get<3>(GetParam());
    args.maxSize = std::get<4>(GetParam());
    args.sizeDistribution = std::get<5>(GetParam());
    UsmRandomMemoryAllocation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    UsmRandomMemoryAllocationTest,
    UsmRandomMemoryAllocationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmRuntimeMemoryPlacement::Host, UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Shared),
        ::testing::Values(128, 1024),
        ::testing::Values(8, MemoryConstants::kiloByte),
        ::testing::Values(4 * MemoryConstants::kiloByte,
                          64 * MemoryConstants::kiloByte,
                          512 * MemoryConstants::kiloByte,
                          4 * MemoryConstants::megaByte),
        ::testing::Values(DistributionKind::Uniform, DistributionKind::LogUniform)));

INSTANTIATE_TEST_SUITE_P(
    UsmRandomMemoryAllocationTestLIMITED,
    UsmRandomMemoryAllocationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmRuntimeMemoryPlacement::Shared),
        ::testing::Values(32),
        ::testing::Values(1024),
        ::testing::Values(4 * MemoryConstants::megaByte),
        ::testing::Values(DistributionKind::LogUniform)));
