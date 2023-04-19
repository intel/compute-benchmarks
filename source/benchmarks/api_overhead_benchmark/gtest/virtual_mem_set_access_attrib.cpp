/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_set_access_attrib.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<VirtualMemSetAccessAttrib> registerTestCase{};

class VirtualMemSetAccessAttribTest : public ::testing::TestWithParam<std::tuple<size_t, std::string>> {
};

TEST_P(VirtualMemSetAccessAttribTest, Test) {
    VirtualMemSetAccessAttribArguments args{};
    args.api = Api::L0;
    args.size = std::get<0>(GetParam());
    args.accessType = std::get<1>(GetParam());

    VirtualMemSetAccessAttrib test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemSetAccessAttribTest,
    VirtualMemSetAccessAttribTest,
    ::testing::Combine(
        ::testing::Values(1 * gigaByte, 3 * gigaByte),
        ::testing::Values("ReadWrite", "ReadOnly", "None")));
