/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_and_copy_pipeline_immediate.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelAndCopyPipelineImmediate> registerTestCase{};

class KernelAndCopyPipelineImmediateTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, size_t, bool, bool>> {
};

TEST_P(KernelAndCopyPipelineImmediateTest, Test) {
    KernelAndCopyPipelineImmediateArguments args;
    args.api = std::get<0>(GetParam());
    args.stages = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.kernelTime = std::get<3>(GetParam());
    args.withCopyOffload = std::get<4>(GetParam());
    args.signalAfterCopy = std::get<5>(GetParam());

    KernelAndCopyPipelineImmediate test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    KernelAndCopyPipelineImmediateTest,
    KernelAndCopyPipelineImmediateTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(8),
        ::testing::Values(32 * megaByte),
        ::testing::Values(7500),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
