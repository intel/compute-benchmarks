/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/new_resources_submission_host.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/memory_constants.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<NewResourcesSubmissionHost> registerTestCase{};

class NewResourcesSubmissionHostTest : public ::testing::TestWithParam<std::tuple<Api, size_t>> {
};

TEST_P(NewResourcesSubmissionHostTest, Test) {
    NewResourcesSubmissionHostArguments args;
    args.api = std::get<0>(GetParam());
    args.size = std::get<1>(GetParam());

    NewResourcesSubmissionHost test;
    test.run(args);
}

using namespace MemoryConstants;
INSTANTIATE_TEST_SUITE_P(
    NewResourcesSubmissionHostTest,
    NewResourcesSubmissionHostTest,
    ::testing::Combine(
        ::CommonGtestArgs::allApis(),
        ::testing::Values(1, 64 * kiloByte, 512 * kiloByte, 1 * megaByte, 16 * megaByte, 64 * megaByte, 256 * megaByte, 1 * gigaByte)));
