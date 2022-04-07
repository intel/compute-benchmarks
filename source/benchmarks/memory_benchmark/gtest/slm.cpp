/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/slm.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<SlmTraffic> registerTestCase{};

class SlmTrafficTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, bool>> {
};

TEST_P(SlmTrafficTest, Test) {
    SlmTrafficArguments args;
    args.api = Api::OpenCL;
    args.size = std::get<0>(GetParam());
    args.occupancyDivider = std::get<1>(GetParam());
    args.writeDirection = std::get<2>(GetParam());

    SlmTraffic test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    SlmTrafficTest,
    SlmTrafficTest,
    ::testing::Combine(
        ::testing::Values(8 * kiloByte),
        ::testing::Values(8, 4, 2, 1),
        ::testing::Values(false)));
