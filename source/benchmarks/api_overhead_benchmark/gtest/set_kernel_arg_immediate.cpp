/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/set_kernel_arg_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<KernelSetArgumentValueImmediate> registerTestCase{};

class KernelSetArgumentValueImmediateTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(KernelSetArgumentValueImmediateTest, Test) {
    KernelSetArgumentValueImmediateArguments args{};
    args.api = Api::L0;
    args.argumentSize = GetParam();

    KernelSetArgumentValueImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSetArgumentValueImmediateTest,
    KernelSetArgumentValueImmediateTest,
    ::testing::Values(8, 64, 256, 512, 1024, 2048));
