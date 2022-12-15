/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<UsmCopyKernel> registerTestCase{};

class UsmCopyKernelTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, DeviceSelection, DeviceSelection, DeviceSelection, size_t, bool>> {};

TEST_P(UsmCopyKernelTest, Test) {
    UsmCopyKernelArguments args;
    args.api = std::get<0>(GetParam());
    args.contextPlacement = std::get<1>(GetParam());
    args.queuePlacement = std::get<2>(GetParam());
    args.srcPlacement = std::get<3>(GetParam());
    args.dstPlacement = std::get<4>(GetParam());
    args.size = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());

    if (!args.validateArgumentsExtra()) {
        GTEST_SKIP(); // If above arguments make no sense (e.g. queue created outside of the context), skip the case
    }

    UsmCopyKernel test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyKernelTest,
    UsmCopyKernelTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::CommonGtestArgs::contextDeviceSelections(),
        ::CommonGtestArgs::resourceDeviceSelections(),
        ::CommonGtestArgs::usmDeviceSelections(),
        ::CommonGtestArgs::usmDeviceSelections(),
        ::testing::Values(512 * megaByte),
        ::testing::Values(true)));
