/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_shared_first_cpu_access.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<UsmSharedFirstCpuAccess> registerTestCase{};

class UsmSharedFirstCpuAccessTest : public ::testing::TestWithParam<std::tuple<Api, UsmInitialPlacement, size_t>> {
};

TEST_P(UsmSharedFirstCpuAccessTest, Test) {
    UsmSharedFirstCpuAccessArguments args;
    args.api = std::get<0>(GetParam());
    args.initialPlacement = std::get<1>(GetParam());
    args.bufferSize = std::get<2>(GetParam());

    UsmSharedFirstCpuAccess test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmSharedFirstCpuAccessTest,
    UsmSharedFirstCpuAccessTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(UsmInitialPlacement::Any, UsmInitialPlacement::Host, UsmInitialPlacement::Device),
        ::testing::Values(64 * megaByte, 128 * megaByte)));
