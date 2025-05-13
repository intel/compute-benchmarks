/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/set_kernel_arg_immediate.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSetArgumentValueImmediate> registerTestCase{};

class KernelSetArgumentValueImmediateTest : public ::testing::TestWithParam<std::tuple<bool, size_t>> {
};

TEST_P(KernelSetArgumentValueImmediateTest, Test) {
    KernelSetArgumentValueImmediateArguments args{};
    args.api = Api::L0;
    args.differentValues = std::get<0>(GetParam());
    args.argumentSize = std::get<1>(GetParam());

    KernelSetArgumentValueImmediate test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSetArgumentValueImmediateTest,
    KernelSetArgumentValueImmediateTest,
    ::testing::Combine(
        ::testing::Bool(),
        ::testing::Values(8, 64, 256, 512, 1024, 2048)));
