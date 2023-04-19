/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_event.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelWithEvent> registerTestCase{};

class KernelWithEventTest : public ::testing::TestWithParam<std::tuple<size_t, size_t, size_t, bool, bool, bool>> {
};

TEST_P(KernelWithEventTest, Test) {
    KernelWithEventArguments args{};

    args.api = Api::L0;
    args.measuredCommands = std::get<0>(GetParam());
    args.workgroupCount = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.useHostSignalEvent = std::get<3>(GetParam());
    args.useDeviceWaitEvent = std::get<4>(GetParam());
    args.useTimestampEvent = std::get<5>(GetParam());

    KernelWithEvent test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelWithEventTest,
    KernelWithEventTest,
    ::testing::Combine(
        ::testing::Values(500),
        ::testing::Values(1, 10, 100),
        ::testing::Values(1),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));
