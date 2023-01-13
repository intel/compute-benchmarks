/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/set_kernel_group_size.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<SetKernelGroupSize> registerTestCase{};

class SetKernelGroupSizeTest : public ::testing::TestWithParam<bool> {};

TEST_P(SetKernelGroupSizeTest, Test) {
    SetKernelGroupSizeArguments args{};
    args.api = Api::L0;
    args.asymmetricLocalWorkSize = GetParam();

    SetKernelGroupSize test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SetKernelGroupSizeTest,
    SetKernelGroupSizeTest,
    ::testing::Values(false, true));
