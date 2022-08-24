/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_staging_buffers.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"
static const inline RegisterTestCase<UsmCopyStagingBuffers> registerTestCase{};

#include <gtest/gtest.h>

class UsmCopyStagingBuffersTest : public ::testing::TestWithParam<std::tuple<Api, bool, UsmMemoryPlacement, size_t, size_t>> {
};

TEST_P(UsmCopyStagingBuffersTest, Test) {
    UsmCopyStagingBuffersArguments args;
    args.api = std::get<0>(GetParam());
    args.forceBlitter = std::get<1>(GetParam());
    args.dstPlacement = std::get<2>(GetParam());
    args.size = std::get<3>(GetParam());
    args.chunks = std::get<4>(GetParam());

    UsmCopyStagingBuffers test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyStagingBuffersTest,
    UsmCopyStagingBuffersTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(false, true),
        ::testing::Values(UsmMemoryPlacement::Device, UsmMemoryPlacement::Host),
        ::testing::Values(1 * kiloByte, 2 * kiloByte, 4 * kiloByte, 128 * kiloByte, 1 * megaByte, 2 * megaByte, 16 * megaByte, 32 * megaByte, 128 * megaByte, 512 * megaByte),
        ::testing::Values(1, 2, 4, 8)));
