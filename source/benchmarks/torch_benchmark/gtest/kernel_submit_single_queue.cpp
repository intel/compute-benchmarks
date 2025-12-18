/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_single_queue.h"

#include "framework/enum/data_type.h"
#include "framework/enum/kernel_name.h"
#include "framework/enum/kernel_submit_pattern.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>

[[maybe_unused]] static const inline RegisterTestCase<KernelSubmitSingleQueue> registerTestCase{};

class KernelSubmitSingleQueueTest : public ::testing::TestWithParam<std::tuple<Api, DataType, KernelName, uint32_t, size_t, KernelSubmitPattern, uint32_t, uint32_t>> {
};

TEST_P(KernelSubmitSingleQueueTest, Test) {
    KernelSubmitSingleQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.KernelDataType = std::get<1>(GetParam());
    args.KernelName = std::get<2>(GetParam());
    args.KernelParamsNum = std::get<3>(GetParam());
    args.KernelBatchSize = std::get<4>(GetParam());
    args.KernelSubmitPattern = std::get<5>(GetParam());
    args.KernelWGCount = std::get<6>(GetParam());
    args.KernelWGSize = std::get<7>(GetParam());
    KernelSubmitSingleQueue test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestAddKernel,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::Int32, DataType::Double, DataType::Float, DataType::Mixed), // KernelDataType
        ::testing::Values(KernelName::Add),                                                     // KernelName
        ::testing::Values(1u, 5u, 10u),                                                         // KernelParamsNum
        ::testing::Values(10u),                                                                 // KernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single,
                          KernelSubmitPattern::D2h_after_batch,
                          KernelSubmitPattern::H2d_before_batch), // KernelSubmitPattern
        ::testing::Values(512u),                                  // KernelWGCount
        ::testing::Values(256u)));                                // KernelWGSize

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestCopyableObject,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::CopyableObject), // KernelDataType
        ::testing::Values(KernelName::Add),          // KernelName
        ::testing::Values(1u),                       // KernelParamsNum
        ::testing::Values(10u),                      // KernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single,
                          KernelSubmitPattern::D2h_after_batch,
                          KernelSubmitPattern::H2d_before_batch), // KernelSubmitPattern
        ::testing::Values(512u),                                  // KernelWGCount
        ::testing::Values(256u)));                                // KernelWGSize

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestEmptyKernel,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::Int32),             // KernelDataType
        ::testing::Values(KernelName::Empty),           // KernelName
        ::testing::Values(1u),                          // KernelParamsNum
        ::testing::Values(10u),                         // KernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single), // KernelSubmitPattern
        ::testing::Values(512u),                        // KernelWGCount
        ::testing::Values(256u)));                      // KernelWGSize
