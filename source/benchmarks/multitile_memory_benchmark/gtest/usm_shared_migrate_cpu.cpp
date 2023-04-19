/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_shared_migrate_cpu.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmSharedMigrateCpu> registerTestCase{};

class UsmSharedMigrateCpuTest : public ::testing::TestWithParam<std::tuple<Api, DeviceSelection, DeviceSelection, size_t, bool>> {};

TEST_P(UsmSharedMigrateCpuTest, Test) {
    UsmSharedMigrateCpuArguments args;
    args.api = std::get<0>(GetParam());
    args.contextPlacement = std::get<1>(GetParam());
    args.bufferPlacement = std::get<2>(GetParam());
    args.bufferSize = std::get<3>(GetParam());
    args.accessAllBytes = std::get<4>(GetParam());

    if (!args.validateArgumentsExtra()) {
        GTEST_SKIP(); // If above arguments make no sense (e.g. queue created outside of the context), skip the case
    }

    UsmSharedMigrateCpu test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmSharedMigrateCpuTest,
    UsmSharedMigrateCpuTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::CommonGtestArgs::contextDeviceSelections(),
        ::CommonGtestArgs::usmSharedSelections(),
        ::testing::Values(128 * megaByte, 512 * megaByte),
        ::testing::Values(false, true)));
