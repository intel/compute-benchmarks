/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/reduction5.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Reduction5> registerTestCase{};

class Reduction5Tests : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(Reduction5Tests, Test) {
    ReductionArguments5 args;
    args.api = std::get<0>(GetParam());
    args.numberOfElements = std::get<1>(GetParam());

    Reduction5 test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    Reduction5Tests,
    Reduction5Tests,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128000000, 256000000, 512000000)));

INSTANTIATE_TEST_SUITE_P(
    Reduction5TestsLIMITED,
    Reduction5Tests,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL),
        ::testing::Values(512000000)));
