/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_bidirectional_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmBidirectionalCopy> registerTestCase{};

class UsmBidirectionalCopyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool>> {};

TEST_P(UsmBidirectionalCopyTest, Test) {
    UsmBidirectionalCopyArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.write = std::get<2>(GetParam());
    args.forceBlitter = std::get<3>(GetParam());

    UsmBidirectionalCopy test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmBidirectionalCopyTest,
    UsmBidirectionalCopyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(512 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
