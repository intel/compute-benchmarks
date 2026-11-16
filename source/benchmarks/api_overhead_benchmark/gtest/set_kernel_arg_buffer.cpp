/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/set_kernel_arg_buffer.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSetArgumentValueBuffer> registerTestCase{};

class KernelSetArgumentValueBufferTest : public ::testing::TestWithParam<std::tuple<uint32_t, bool>> {
};

TEST_P(KernelSetArgumentValueBufferTest, Test) {
    KernelSetArgumentValueBufferArguments args{};
    args.api = Api::L0;
    args.argumentIndex = std::get<0>(GetParam());
    args.differentValues = std::get<1>(GetParam());

    KernelSetArgumentValueBuffer test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSetArgumentValueBufferTest,
    KernelSetArgumentValueBufferTest,
    ::testing::Combine(
        ::testing::Values(0u, 7u),
        ::testing::Bool()));
