/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/in_order_wait_append.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<InOrderWaitAppend> registerTestCase{};

class InOrderWaitAppendTest : public ::testing::TestWithParam<std::tuple<bool>> {
};

TEST_P(InOrderWaitAppendTest, Test) {
    InOrderWaitAppendArguments args{};
    args.api = Api::L0;
    args.counterBasedEvents = std::get<0>(GetParam());
    InOrderWaitAppend test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    InOrderWaitAppendTest,
    InOrderWaitAppendTest,
    ::testing::Combine(
        ::testing::Values(false, true)));