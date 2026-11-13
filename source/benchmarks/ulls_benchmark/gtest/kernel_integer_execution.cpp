/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_integer_execution.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelIntegerExecution> registerTestCase{};

class KernelIntegerExecutionTest : public ::testing::TestWithParam<UsmRuntimeMemoryPlacement> {
};

TEST_P(KernelIntegerExecutionTest, Test) {
    KernelIntegerExecutionArguments args{};
    args.api = Api::L0;
    args.placement = GetParam();

    KernelIntegerExecution test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelIntegerExecutionTest,
    KernelIntegerExecutionTest,
    ::testing::Values(UsmRuntimeMemoryPlacement::Device, UsmRuntimeMemoryPlacement::Host, UsmRuntimeMemoryPlacement::Shared));
