/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/command_list_get_device_handle.h"

#include "framework/test_case/register_test_case.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CommandListGetDeviceHandle> registerTestCase{};

class CommandListGetDeviceHandleTest : public ::testing::TestWithParam<bool> {
};

TEST_P(CommandListGetDeviceHandleTest, Test) {
    CommandListQueryArguments args{};
    args.api = Api::L0;
    args.useImmediate = GetParam();

    CommandListGetDeviceHandle test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CommandListGetDeviceHandleTest,
    CommandListGetDeviceHandleTest,
    ::testing::Values(false, true));
