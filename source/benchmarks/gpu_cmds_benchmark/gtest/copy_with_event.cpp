/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/copy_with_event.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<CopyWithEvent> registerTestCase{};

class CopyWithEventTest : public ::testing::TestWithParam<std::tuple<size_t, bool, bool, bool>> {
};

TEST_P(CopyWithEventTest, Test) {
    CopyWithEventArguments args{};

    args.api = Api::L0;
    args.measuredCommands = std::get<0>(GetParam());
    args.useHostSignalEvent = std::get<1>(GetParam());
    args.useDeviceWaitEvent = std::get<2>(GetParam());
    args.useTimestampEvent = std::get<3>(GetParam());

    CopyWithEvent test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    CopyWithEventTest,
    CopyWithEventTest,
    ::testing::Combine(
        ::testing::Values(500),
        ::testing::Values(false, true),
        ::testing::Values(false, true),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    CopyWithEventTestLIMITED,
    CopyWithEventTest,
    ::testing::ValuesIn([] {
        std::vector<std::tuple<size_t, bool, bool, bool>> testCases;
        testCases.emplace_back(500, false, false, true);
        testCases.emplace_back(500, false, true, false);
        testCases.emplace_back(500, true, false, false);
        testCases.emplace_back(500, true, true, true);
        return testCases;
    }()));
