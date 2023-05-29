/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/remote_access_max_saturation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RemoteAccessMaxSaturation> registerTestCase{};

class RemoteAccessMaxSaturationTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, bool, size_t, size_t>> {
};

TEST_P(RemoteAccessMaxSaturationTest, Test) {
    RemoteAccessMaxSaturationArguments args;
    args.api = std::get<0>(GetParam());
    args.remoteFraction = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    args.workItemPackSize = std::get<4>(GetParam());
    args.writesPerWorkgroup = std::get<5>(GetParam());

    RemoteAccessMaxSaturation test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    RemoteAccessMaxSaturationTest,
    RemoteAccessMaxSaturationTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 2, 4, 6, 8, 10, 20, 50, 100, 0),
        ::testing::Values(1 * gigaByte),
        ::testing::Values(true),
        ::testing::Values(64),
        ::testing::Values(1, 4, 8, 16, 32, 64, 128, 256, 512, 1024)));
