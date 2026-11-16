/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/simple_add.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SimpleAdd> registerTestCase{};

class SimpleAddTest : public ::testing::TestWithParam<Api> {
};

TEST_P(SimpleAddTest, Test) {
    SimpleAddArguments args{};
    args.api = GetParam();

    SimpleAdd test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SimpleAddTest,
    SimpleAddTest,
    ::testing::Values(Api::OpenCL, Api::L0));
