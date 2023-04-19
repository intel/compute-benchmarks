/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/get_memory_properties.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<GetMemoryProperties> registerTestCase{};

class GetMemoryPropertiesTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(GetMemoryPropertiesTest, Test) {
    GetMemoryPropertiesArguments args{};
    args.api = Api::L0;
    args.AllocationsCount = GetParam();

    GetMemoryProperties test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    GetMemoryPropertiesTest,
    GetMemoryPropertiesTest,
    ::testing::Values(1, 10u, 100u, 1000u));
