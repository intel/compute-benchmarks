/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_immediate_after_kernels.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/usm_copy_direction_skip.h"

[[maybe_unused]] static const inline RegisterTestCase<UsmCopyImmediateAfterKernels> registerTestCase{};

#include <gtest/gtest.h>

class UsmCopyImmediateAfterKernelsTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, size_t, size_t, UsmMemoryPlacement, UsmMemoryPlacement>> {
};

TEST_P(UsmCopyImmediateAfterKernelsTest, Test) {
    UsmCopyImmediateAfterKernelsArguments args;
    args.api = Api::L0;
    args.numKernels = std::get<0>(GetParam());
    args.numCopies = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.kernelTime = std::get<3>(GetParam());
    args.sourcePlacement = std::get<4>(GetParam());
    args.destinationPlacement = std::get<5>(GetParam());

    if (shouldSkipCopyDirection(args.sourcePlacement, args.destinationPlacement)) {
        GTEST_SKIP();
    }

    UsmCopyImmediateAfterKernels test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyImmediateAfterKernelsTest,
    UsmCopyImmediateAfterKernelsTest,
    ::testing::Combine(
        ::testing::Values(0, 4),
        ::testing::Values(11),
        ::testing::Values(1 * kiloByte, 4 * kiloByte, 16 * kiloByte),
        ::testing::Values(200),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::deviceAndHost),
        ::testing::ValuesIn(UsmMemoryPlacementArgument::deviceAndHost)));
