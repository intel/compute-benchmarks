/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/get_memory_properties_with_offseted_pointer.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<GetMemoryPropertiesWithOffsetedPointer> registerTestCase{};

class GetMemoryPropertiesWithOffsetedPointerTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(GetMemoryPropertiesWithOffsetedPointerTest, Test) {
    GetMemoryPropertiesWithOffsetedPointerArguments args{};
    args.api = Api::L0;
    args.AllocationsCount = std::get<0>(GetParam());
    GetMemoryPropertiesWithOffsetedPointer test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    GetMemoryPropertiesWithOffsetedPointerTest,
    GetMemoryPropertiesWithOffsetedPointerTest,
    ::testing::Values(1, 10u, 100u, 1000u, 10000u));
