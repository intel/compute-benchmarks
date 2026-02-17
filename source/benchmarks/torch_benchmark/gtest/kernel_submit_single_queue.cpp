/*
 * Copyright (C) 2025-2026 Intel Corporation
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

class KernelSubmitSingleQueueTest : public ::testing::TestWithParam<std::tuple<Api, DataType, KernelName, uint32_t, size_t, KernelSubmitPattern, uint32_t, uint32_t, bool>> {
};

TEST_P(KernelSubmitSingleQueueTest, Test) {
    KernelSubmitSingleQueueArguments args{};
    args.api = std::get<0>(GetParam());
    args.kernelDataType = std::get<1>(GetParam());
    args.kernelName = std::get<2>(GetParam());
    args.kernelParamsNum = std::get<3>(GetParam());
    args.kernelBatchSize = std::get<4>(GetParam());
    args.kernelSubmitPattern = std::get<5>(GetParam());
    args.kernelWGCount = std::get<6>(GetParam());
    args.kernelWGSize = std::get<7>(GetParam());
    args.useEvents = std::get<8>(GetParam());
    KernelSubmitSingleQueue test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestAddKernel,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::Int32, DataType::Double, DataType::Float, DataType::Mixed), // kernelDataType
        ::testing::Values(KernelName::Add),                                                     // kernelName
        ::testing::Values(1u, 5u, 10u),                                                         // kernelParamsNum
        ::testing::Values(10u),                                                                 // kernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single,
                          KernelSubmitPattern::D2h_after_batch,
                          KernelSubmitPattern::H2d_before_batch), // kernelSubmitPattern
        ::testing::Values(512u),                                  // kernelWGCount
        ::testing::Values(256u),                                  // kernelWGSize
        ::testing::Values(false)));                               // useEvents

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestCopyableObject,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::CopyableObject), // kernelDataType
        ::testing::Values(KernelName::Add),          // kernelName
        ::testing::Values(1u),                       // kernelParamsNum
        ::testing::Values(10u),                      // kernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single,
                          KernelSubmitPattern::D2h_after_batch,
                          KernelSubmitPattern::H2d_before_batch), // kernelSubmitPattern
        ::testing::Values(512u),                                  // kernelWGCount
        ::testing::Values(256u),                                  // kernelWGSize
        ::testing::Values(true)));                                // useEvents

INSTANTIATE_TEST_SUITE_P(
    KernelSubmitSingleQueueTestEmptyKernel,
    KernelSubmitSingleQueueTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL, Api::SYCLPREVIEW, Api::L0),
        ::testing::Values(DataType::Int32),             // kernelDataType
        ::testing::Values(KernelName::Empty),           // kernelName
        ::testing::Values(1u),                          // kernelParamsNum
        ::testing::Values(10u),                         // kernelBatchSize
        ::testing::Values(KernelSubmitPattern::Single), // kernelSubmitPattern
        ::testing::Values(512u),                        // kernelWGCount
        ::testing::Values(256u),                        // kernelWGSize
        ::testing::Values(false)));                     // useEvents
