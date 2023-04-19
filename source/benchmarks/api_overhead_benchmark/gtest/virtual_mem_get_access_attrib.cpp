/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_get_access_attrib.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<VirtualMemGetAccessAttrib> registerTestCase{};

class VirtualMemGetAccessAttribTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(VirtualMemGetAccessAttribTest, Test) {
    VirtualMemGetAccessAttribArguments args{};
    args.api = Api::L0;
    args.size = GetParam();

    VirtualMemGetAccessAttrib test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    VirtualMemGetAccessAttribTest,
    VirtualMemGetAccessAttribTest,
    ::testing::Values(1 * gigaByte, 3 * gigaByte));
