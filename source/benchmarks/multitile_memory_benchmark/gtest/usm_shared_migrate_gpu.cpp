/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_shared_migrate_gpu.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<UsmSharedMigrateGpu> registerTestCase{};

class UsmSharedMigrateGpuTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, DeviceSelection, size_t>> {};

TEST_P(UsmSharedMigrateGpuTest, Test) {
    UsmSharedMigrateGpuArguments args;
    args.api = std::get<0>(GetParam());
    args.contextPlacement = std::get<1>(GetParam());
    args.bufferPlacement = std::get<2>(GetParam());
    args.bufferSize = std::get<3>(GetParam());

    if (!args.validateArgumentsExtra()) {
        GTEST_SKIP(); // If above arguments make no sense (e.g. queue created outside of the context), skip the case
    }

    UsmSharedMigrateGpu test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmSharedMigrateGpuTest,
    UsmSharedMigrateGpuTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::CommonGtestArgs::contextDeviceSelections(),
        ::CommonGtestArgs::usmSharedSelections(),
        ::testing::Values(128 * megaByte, 512 * megaByte)));
