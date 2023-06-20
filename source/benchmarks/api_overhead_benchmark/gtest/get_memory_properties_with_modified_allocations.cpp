/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/get_memory_properties_with_modified_allocations.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<GetMemoryPropertiesWithModifiedAllocations> registerTestCase{};

class GetMemoryPropertiesWithModifiedAllocationsTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(GetMemoryPropertiesWithModifiedAllocationsTest, Test) {
    GetMemoryPropertiesWithModifiedAllocationsArguments args{};
    args.api = Api::L0;
    args.AllocationsCount = std::get<0>(GetParam());
    GetMemoryPropertiesWithModifiedAllocations test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    GetMemoryPropertiesWithModifiedAllocationsTest,
    GetMemoryPropertiesWithModifiedAllocationsTest,
    ::testing::Values(1, 10u, 100u, 1000u, 10000u));
