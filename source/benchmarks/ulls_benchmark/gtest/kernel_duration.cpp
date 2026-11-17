/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_duration.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelDuration> registerTestCase{};

class KernelDurationTest : public ::testing::TestWithParam<Api> {
};

TEST_P(KernelDurationTest, Test) {
    KernelDurationArguments args{};
    args.api = GetParam();

    KernelDuration test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelDurationTest,
    KernelDurationTest,
    ::testing::Values(Api::L0));
