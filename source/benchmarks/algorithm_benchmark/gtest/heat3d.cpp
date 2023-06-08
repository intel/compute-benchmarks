/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/heat3d.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<Heat3D> registerTestCase{};

class Heat3DTest : public ::testing::TestWithParam<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t>> {
};

TEST_P(Heat3DTest, Test) {
    Heat3DArguments args{};
    args.api = Api::L0;
    args.subDomainX = std::get<0>(GetParam());
    args.subDomainY = std::get<1>(GetParam());
    args.subDomainZ = std::get<2>(GetParam());
    args.timesteps = std::get<3>(GetParam());
    args.meshLength = std::get<4>(GetParam());

    Heat3D test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    Heat3DTest,
    Heat3DTest,
    ::testing::Combine(
        ::testing::Values(1, 2), ::testing::Values(1, 2), ::testing::Values(1, 2), ::testing::Values(100), ::testing::Values(16, 256)));
