/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/random_access_multi_resource.h"

#include "framework/enum/measurement_type.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RandomAccessMultiResource> registerTestCase{};

class RandomAccessMultiResourceTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, UsmMemoryPlacement, UsmMemoryPlacement>> {
};

TEST_P(RandomAccessMultiResourceTest, Test) {
    RandomAccessMultiResourceArguments args;
    args.api = Api::L0;
    args.firstSize = std::get<0>(GetParam());
    args.secondSize = std::get<1>(GetParam());
    args.firstPlacement = std::get<2>(GetParam());
    args.secondPlacement = std::get<3>(GetParam());

    RandomAccessMultiResource test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    RandomAccessMultiResourceTest,
    RandomAccessMultiResourceTest,
    ::testing::Combine(
        ::testing::Values(1 * megaByte, 2 * megaByte, 8 * megaByte + 300 * kiloByte),
        ::testing::Values(1 * megaByte, 2 * megaByte, 8 * megaByte + 300 * kiloByte),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host)));