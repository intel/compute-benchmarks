/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_timestamp.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<WriteTimestamp> registerTestCase{};

class WriteTimestampTest : public ::testing::TestWithParam<size_t> {
};

TEST_P(WriteTimestampTest, Test) {
    WriteTimestampArguments args{};
    args.api = Api::L0;
    args.measuredCommands = GetParam();

    WriteTimestamp test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    WriteTimestampTest,
    WriteTimestampTest,
    ::testing::Values(500, 1000));
