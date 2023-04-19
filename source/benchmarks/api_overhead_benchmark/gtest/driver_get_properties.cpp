/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/driver_get_properties.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DriverGetProperties> registerTestCase{};

class DriverGetPropertiesTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(DriverGetPropertiesTest, Test) {
    DriverGetPropertiesArguments args{};
    args.api = Api::L0;

    DriverGetProperties test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DriverGetPropertiesTest,
    DriverGetPropertiesTest,
    ::testing::Values(Api::L0));