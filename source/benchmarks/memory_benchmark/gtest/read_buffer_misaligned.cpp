/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/read_buffer_misaligned.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<ReadBufferMisaligned> registerTestCase{};

class ReadBufferMisalignedTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, bool>> {
};

TEST_P(ReadBufferMisalignedTest, Test) {
    ReadBufferMisalignedArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.misalignmentFromCacheline = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    ReadBufferMisaligned test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    ReadBufferMisalignedTest,
    ReadBufferMisalignedTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(16 * megaByte),
        ::testing::Values(0, 1, 2, 4),
        ::testing::Values(false)));
