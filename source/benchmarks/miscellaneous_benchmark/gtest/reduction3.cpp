/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/reduction3.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Reduction3> registerTestCase{};

class Reduction3Tests : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(Reduction3Tests, Test) {
    ReductionArguments3 args;
    args.api = std::get<0>(GetParam());
    args.numberOfElements = std::get<1>(GetParam());

    Reduction3 test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    Reduction3Tests,
    Reduction3Tests,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128000000, 256000000, 512000000)));
