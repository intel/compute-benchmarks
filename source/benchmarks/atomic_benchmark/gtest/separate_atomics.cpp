/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/test_type.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/test_type_skip.h"

#include "definitions/separate_atomic.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SeparateAtomics> registerTestCase{};

class SeparateAtomicsTest : public ::testing::TestWithParam<std::tuple<Api, DataType, MathOperation, size_t, CommonGtestArgs::EnqueueSize, bool, TestType>> {
};

TEST_P(SeparateAtomicsTest, Test) {
    SeparateAtomicsArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());
    args.atomicOperation = std::get<2>(GetParam());
    args.atomicsPerCacheline = std::get<3>(GetParam());
    args.workgroupCount = std::get<4>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<4>(GetParam()).workgroupSize;
    args.useEvents = std::get<5>(GetParam());

    if (args.atomicsPerCacheline > args.workgroupCount * args.workgroupSize) {
        GTEST_SKIP();
    }

    const auto testType = std::get<6>(GetParam());
    if (isTestSkipped(Configuration::get().extended, testType)) {
        GTEST_SKIP();
    }

    SeparateAtomics test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsTest,
    SeparateAtomicsTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::reducedAtomicMathOperations(),
        ::testing::Values(1, 4),
        ::CommonGtestArgs::reducedEnqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsExtendedTest,
    SeparateAtomicsTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::testing::Values(1, 4),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(TestType::Extended)));

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsTestLIMITED,
    SeparateAtomicsTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Int32),
        ::testing::Values(MathOperation::Inc),
        ::testing::Values(1, 4),
        ::testing::Values(CommonGtestArgs::EnqueueSize{32, 64}),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));
