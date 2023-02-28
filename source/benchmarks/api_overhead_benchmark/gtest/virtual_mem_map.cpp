/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_map.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<VirtualMemMap> registerTestCase{};

class VirtualMemMapTest : public ::testing::TestWithParam<std::tuple<size_t, bool, std::string>> {
};

TEST_P(VirtualMemMapTest, Test) {
    VirtualMemMapArguments args{};
    args.api = Api::L0;
    args.reserveSize = std::get<0>(GetParam());
    args.useOffset = std::get<1>(GetParam());
    args.accessType = std::get<2>(GetParam());

    VirtualMemMap test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemMapTest,
    VirtualMemMapTest,
    ::testing::Combine(
        ::testing::Values(1 * gigaByte, 3 * gigaByte),
        ::testing::Values(false, true),
        ::testing::Values("ReadWrite", "ReadOnly", "None")));
