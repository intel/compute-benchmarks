/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/multiple_immediate_with_dependencies.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<MultipleImmediateCmdListsWithDependencies> registerTestCase{};

class MultipleImmediateCmdListsWithDependenciesTests : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(MultipleImmediateCmdListsWithDependenciesTests, Test) {
    MultipleImmediateCmdListsWithDependenciesArguments args{};
    args.api = std::get<0>(GetParam());
    args.cmdlistCount = std::get<1>(GetParam());

    MultipleImmediateCmdListsWithDependencies test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MultipleImmediateCmdListsWithDependenciesTests,
    MultipleImmediateCmdListsWithDependenciesTests,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1, 2, 4, 8)));
