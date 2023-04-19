/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/resource_reassign.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<ResourceReassign> registerTestCase{};

class ResourceReassignTest : public ::testing::TestWithParam<std::tuple<size_t>> {
};

TEST_P(ResourceReassignTest, Test) {
    ResourceReassignArguments args{};
    args.api = Api::OpenCL;
    args.queueCount = std::get<0>(GetParam());

    ResourceReassign test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    ResourceReassignTest,
    ResourceReassignTest,
    ::testing::Combine(
        ::testing::Values(1, 2)));