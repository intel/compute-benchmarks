/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/append_kernel_with_mapped_timestamp_event.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<AppendKernelWithMappedTimestampEvent> registerTestCase{};

class AppendKernelWithMappedTimestampEventTest : public ::testing::TestWithParam<bool> {
};

TEST_P(AppendKernelWithMappedTimestampEventTest, Test) {
    AppendKernelWithMappedTimestampEventArguments args{};
    args.api = Api::L0;
    args.useMappedTimestampEvent = GetParam();

    AppendKernelWithMappedTimestampEvent test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    AppendKernelWithMappedTimestampEventTest,
    AppendKernelWithMappedTimestampEventTest,
    ::testing::Bool());
