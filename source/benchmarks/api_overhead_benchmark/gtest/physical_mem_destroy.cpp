/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/physical_mem_destroy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<PhysicalMemDestroy> registerTestCase{};

class PhysicalMemDestroyTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(PhysicalMemDestroyTest, Test) {
    PhysicalMemDestroyArguments args{};
    args.api = Api::L0;
    PhysicalMemDestroy test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    PhysicalMemDestroyTest,
    PhysicalMemDestroyTest,
    ::testing::Values(Api::L0));
