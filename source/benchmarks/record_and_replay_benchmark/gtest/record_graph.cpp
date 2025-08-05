/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/record_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RecordGraph> registerTestCase{};

class RecordGraphTest : public ::testing::TestWithParam<std::tuple<Api, bool>> {
};

TEST_P(RecordGraphTest, Test) {
    RecordGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.emulate = std::get<1>(GetParam());
    RecordGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    RecordGraphTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(false, true) // emulate
        ));
