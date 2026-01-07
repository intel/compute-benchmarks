/*
 * Copyright (C) 2022-2026 Intel Corporation
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

class UsmSharedMigrateCpuTest : public ::testing::TestWithParam<std::tuple<Api, bool, size_t, UsmMemAdvisePreferredLocation>> {
};

TEST_P(UsmSharedMigrateCpuTest, Test) {
    UsmSharedMigrateCpuArguments args;
    args.api = std::get<0>(GetParam());
    args.accessAllBytes = std::get<1>(GetParam());
    args.bufferSize = std::get<2>(GetParam());
    args.preferredLocation = std::get<3>(GetParam());

    UsmSharedMigrateCpu test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmSharedMigrateCpuTest,
    UsmSharedMigrateCpuTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false, true),
        ::testing::Values(128 * megaByte, 256 * megaByte),
        ::testing::Values(UsmMemAdvisePreferredLocation::None, UsmMemAdvisePreferredLocation::System, UsmMemAdvisePreferredLocation::Device)));
