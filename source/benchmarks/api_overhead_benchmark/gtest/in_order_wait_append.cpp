/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/in_order_wait_append.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<InOrderWaitAppend> registerTestCase{};

class InOrderWaitAppendTest : public ::testing::TestWithParam<std::tuple<bool, bool, size_t>> {
};

TEST_P(InOrderWaitAppendTest, Test) {
    InOrderWaitAppendArguments args{};
    args.api = Api::L0;
    args.counterBasedEvents = std::get<0>(GetParam());
    args.isCompleted = std::get<1>(GetParam());
    args.kernelExecutionTime = std::get<2>(GetParam());
    InOrderWaitAppend test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    InOrderWaitAppendTest,
    InOrderWaitAppendTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(200u)));
