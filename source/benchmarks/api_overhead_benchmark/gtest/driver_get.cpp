/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/driver_get.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DriverGet> registerTestCase{};

class DriverGetTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(DriverGetTest, Test) {
    DriverGetArguments args{};
    args.api = Api::L0;
    args.getDriverCount = GetParam();
    DriverGet test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DriverGetTest,
    DriverGetTest,
    ::testing::Values(0, 1, 4));