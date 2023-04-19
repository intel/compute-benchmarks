/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_fill.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmFill> registerTestCase{};

class UsmFillTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, DeviceSelection, DeviceSelection, size_t, size_t, bool, bool>> {};

TEST_P(UsmFillTest, Test) {
    UsmFillArguments args;
    args.api = std::get<0>(GetParam());
    args.contextPlacement = std::get<1>(GetParam());
    args.queuePlacement = std::get<2>(GetParam());
    args.bufferPlacement = std::get<3>(GetParam());
    args.size = std::get<4>(GetParam());
    args.patternSize = std::get<5>(GetParam());
    args.forceBlitter = std::get<6>(GetParam());
    args.useEvents = std::get<7>(GetParam());

    if (!args.validateArgumentsExtra()) {
        GTEST_SKIP(); // If above arguments make no sense (e.g. queue created outside of the context), skip the case
    }

    UsmFill test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmFillTest,
    UsmFillTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::CommonGtestArgs::contextDeviceSelections(),
        ::CommonGtestArgs::resourceDeviceSelections(),
        ::CommonGtestArgs::usmDeviceSelections(),
        ::testing::Values(512 * megaByte),
        ::testing::Values(1, 128),
        ::testing::Values(false, true),
        ::testing::Values(true)));
