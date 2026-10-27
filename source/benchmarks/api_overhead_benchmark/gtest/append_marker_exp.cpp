/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_marker_exp.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendMarkerExp> registerTestCase{};

class AppendMarkerExpTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(AppendMarkerExpTest, Test) {
    AppendMarkerExpArguments args{};
    args.api = Api::L0;
    args.appendCount = GetParam();

    AppendMarkerExp test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendMarkerExpTest,
    AppendMarkerExpTest,
    ::testing::Values(100));
