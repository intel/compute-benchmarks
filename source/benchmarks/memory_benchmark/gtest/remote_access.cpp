/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/remote_access.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<RemoteAccess> registerTestCase{};

class RemoteAccessTest : public ::testing::TestWithParam<std::tuple<Api, size_t, size_t, bool, size_t>> {
};

TEST_P(RemoteAccessTest, Test) {
    RemoteAccessArguments args;
    args.api = std::get<0>(GetParam());
    args.remoteFraction = std::get<1>(GetParam());
    args.size = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());
    args.workItemPackSize = std::get<4>(GetParam());

    RemoteAccess test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    RemoteAccessTest,
    RemoteAccessTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 2, 4, 6, 8, 10, 20, 50, 100, 0),
        ::testing::Values(1 * gigaByte),
        ::testing::Values(true),
        ::testing::Values(1, 8, 32, 64)));
