/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_reserve.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<VirtualMemReserve> registerTestCase{};

class VirtualMemReserveTest : public ::testing::TestWithParam<std::tuple<size_t, bool>> {
};

TEST_P(VirtualMemReserveTest, Test) {
    VirtualMemReserveArguments args{};
    args.api = Api::L0;
    args.reserveSize = std::get<0>(GetParam());
    args.useNull = std::get<1>(GetParam());

    VirtualMemReserve test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemReserveTest,
    VirtualMemReserveTest,
    ::testing::Combine(
        ::testing::Values(64 * kiloByte, 1 * gigaByte, 3 * gigaByte),
        ::testing::Values(true)));
