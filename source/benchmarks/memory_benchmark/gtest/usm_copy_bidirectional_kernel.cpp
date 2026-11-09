/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/usm_copy_bidirectional_kernel.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"
[[maybe_unused]] static const inline RegisterTestCase<UsmCopyBidirectionalKernel> registerTestCase{};

#include <gtest/gtest.h>

class UsmCopyBidirectionalKernelTest : public ::testing::TestWithParam<std::tuple<Api, size_t, bool>> {
};

TEST_P(UsmCopyBidirectionalKernelTest, Test) {
    UsmCopyBidirectionalKernelArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());
    args.useEvents = std::get<2>(GetParam());

    UsmCopyBidirectionalKernel test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    UsmCopyBidirectionalKernelTest,
    UsmCopyBidirectionalKernelTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(256 * megaByte),
        ::testing::Values(true)));
