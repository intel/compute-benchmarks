/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/sobel.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Sobel> registerTestCase{};

class SobelTest : public ::testing::TestWithParam<Api> {
};

TEST_P(SobelTest, Test) {
    SobelArguments args{};
    args.api = GetParam();

    Sobel test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SobelTest,
    SobelTest,
    ::testing::Values(Api::OpenCL, Api::L0));
