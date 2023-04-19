/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/driver_get_api_version.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DriverGetApiVersion> registerTestCase{};

class DriverGetApiVersionTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(DriverGetApiVersionTest, Test) {
    DriverGetApiVersionArguments args{};
    args.api = Api::L0;

    DriverGetApiVersion test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DriverGetApiVersionTest,
    DriverGetApiVersionTest,
    ::testing::Values(Api::L0));