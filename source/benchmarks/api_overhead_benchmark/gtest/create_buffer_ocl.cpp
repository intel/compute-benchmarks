/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include "definitions/create_buffer.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<CreateBuffer> registerTestCase{};

class CreateBufferTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, bool, bool, bool>> {
};

TEST_P(CreateBufferTest, Test) {
    CreateBufferArguments args{};
    args.api = std::get<0>(GetParam());
    args.bufferSize = std::get<1>(GetParam());
    args.allocateAll = std::get<2>(GetParam());
    args.readOnly = std::get<3>(GetParam());
    args.copyHostPtr = std::get<4>(GetParam());
    args.forceHostMemoryIntel = std::get<5>(GetParam());

    CreateBuffer test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CreateBufferTest,
    CreateBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(64, 10240),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
