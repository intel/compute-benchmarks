/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/reduction.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<Reduction> registerTestCase{};

class ReductionTest : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(ReductionTest, Test) {
    ReductionArguments args;
    args.api = std::get<0>(GetParam());
    args.numberOfElements = std::get<1>(GetParam());

    Reduction test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    ReductionTest,
    ReductionTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128000000, 256000000, 512000000)));
