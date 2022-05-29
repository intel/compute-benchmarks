/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_eu_copy.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"
static const inline RegisterTestCase<UsmEUCopy> registerTestCase{};

#include <gtest/gtest.h>

class UsmEUCopyTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, BufferContents, bool, bool>> {
};

TEST_P(UsmEUCopyTest, Test) {
    UsmP2PCopyArguments args;
    args.api = std::get<0>(GetParam());
    args.srcDeviceId = std::get<1>(GetParam());
    args.dstDeviceId = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.contents = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());
    args.reuseCommandList = std::get<6>(GetParam());

    if (args.srcDeviceId == args.dstDeviceId) {
        GTEST_SKIP();
    }

    UsmEUCopy test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmEUCopyTest,
    UsmEUCopyTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(0, 1),
        ::testing::Values(0, 1),
        ::testing::Values(512 * megaByte),
        ::testing::Values(BufferContents::Random),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
