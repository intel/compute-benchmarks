/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_event_record_query.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitEventRecordQuery> registerTestCase{};

class KernelSubmitEventRecordQueryTest : public ::testing::TestWithParam<std::tuple<Api, uint32_t, uint32_t, uint32_t, bool>> {
};

TEST_P(KernelSubmitEventRecordQueryTest, Test) {
    KernelSubmitEventRecordQueryArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelWGCount = std::get<1>(GetParam());
    args.kernelWGSize = std::get<2>(GetParam());
    args.eventQueryIterations = std::get<3>(GetParam());
    args.useProfiling = std::get<4>(GetParam());

    KernelSubmitEventRecordQuery test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitEventRecordQueryTest,
    KernelSubmitEventRecordQueryTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(512),     // kernelWGCount
        ::testing::Values(256),     // kernelWGSize
        ::testing::Values(1000),    // eventQueryIterations
        ::testing::Values(false))); // useProfiling
