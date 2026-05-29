/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_update_mutable_command_kernels.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListUpdateMutableCommandKernels> registerTestCase{};

class CommandListUpdateMutableCommandKernelsTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(CommandListUpdateMutableCommandKernelsTest, Test) {
    CommandListUpdateMutableCommandKernelsArguments args{};
    args.api = Api::L0;
    args.numKernels = GetParam();

    CommandListUpdateMutableCommandKernels test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListUpdateMutableCommandKernelsTest,
    CommandListUpdateMutableCommandKernelsTest,
    ::testing::Values(2u, 10u));
