/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_shared_migrate_gpu.h"

#include "framework/argument/basic_argument.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmSharedMigrateGpu> registerTestCase{};

class UsmSharedMigrateGpuTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool, UsmMemAdvisePreferredLocation>> {
};

TEST_P(UsmSharedMigrateGpuTest, Test) {
    UsmSharedMigrateGpuArguments args;
    args.api = std::get<0>(GetParam());
    args.bufferSize = std::get<1>(GetParam());
    args.prefetchMemory = std::get<2>(GetParam());
    args.preferredLocation = std::get<3>(GetParam());

    UsmSharedMigrateGpu test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmSharedMigrateGpuTest,
    UsmSharedMigrateGpuTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(128 * megaByte, 256 * megaByte),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemAdvisePreferredLocation::None, UsmMemAdvisePreferredLocation::System, UsmMemAdvisePreferredLocation::Device)));
