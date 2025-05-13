/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/fill_buffer.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<FillBuffer> registerTestCase{};

class FillBufferTest : public ::testing::TestWithParam<std::tuple<DeviceSelection, DeviceSelection, DeviceSelection, size_t, size_t, bool, bool, bool>> {};

TEST_P(FillBufferTest, Test) {
    FillBufferArguments args;
    args.api = Api::OpenCL;
    args.contextPlacement = std::get<0>(GetParam());
    args.queuePlacement = std::get<1>(GetParam());
    args.bufferPlacement = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.patternSize = std::get<4>(GetParam());
    args.compressed = std::get<5>(GetParam());
    args.forceBlitter = std::get<6>(GetParam());
    args.useEvents = std::get<7>(GetParam());

    if (!args.validateArgumentsExtra()) {
        GTEST_SKIP(); // If above arguments make no sense (e.g. queue created outside of the context), skip the case
    }

    FillBuffer test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    FillBufferTest,
    FillBufferTest,
    ::testing::Combine(
        ::CommonGtestArgs::contextDeviceSelections(),
        ::CommonGtestArgs::resourceDeviceSelections(),
        ::CommonGtestArgs::resourceDeviceSelections(),
        ::testing::Values(128 * megaByte, 512 * megaByte),
        ::testing::Values(1, 16, 128),
        ::testing::Values(false, true),
        ::testing::Values(true),
        ::testing::Values(true)));

INSTANTIATE_TEST_SUITE_P(
    FillBufferTestLIMITED,
    FillBufferTest,
    ::testing::Combine(
        ::testing::Values(DeviceSelection::Root),
        ::testing::Values(DeviceSelection::Root),
        ::testing::Values(DeviceSelection::Host),
        ::testing::Values(512 * megaByte),
        ::testing::Values(128),
        ::testing::Values(false),
        ::testing::Values(true),
        ::testing::Values(true)));
