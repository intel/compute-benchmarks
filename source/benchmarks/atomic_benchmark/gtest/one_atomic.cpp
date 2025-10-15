/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/one_atomic.h"

#include "framework/enum/test_type.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/test_type_skip.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<OneAtomic> registerTestCase{};

class OneAtomicTest : public ::testing::TestWithParam<std::tuple<Api, DataType, MathOperation, CommonGtestArgs::EnqueueSize, bool, TestType>> {
};

TEST_P(OneAtomicTest, Test) {
    OneAtomicArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());
    args.atomicOperation = std::get<2>(GetParam());
    args.workgroupCount = std::get<3>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<3>(GetParam()).workgroupSize;
    args.useEvents = std::get<4>(GetParam());

    const auto testType = std::get<5>(GetParam());
    if (isTestSkipped(Configuration::get().extended, testType)) {
        GTEST_SKIP();
    }

    OneAtomic test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    OneAtomicTest,
    OneAtomicTest,
    ::testing::Combine(::testing::Values(Api::OpenCL, Api::L0),
                       ::testing::Values(DataType::Float, DataType::Int32),
                       ::CommonGtestArgs::reducedAtomicMathOperations(),
                       ::CommonGtestArgs::reducedEnqueueSizesForAtomics(),
                       ::testing::Values(true),
                       ::testing::Values(TestType::Regular)));

INSTANTIATE_TEST_SUITE_P(
    OneAtomicExtendedTest,
    OneAtomicTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(TestType::Extended)));

INSTANTIATE_TEST_SUITE_P(
    OneAtomicTestLIMITED,
    OneAtomicTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Int32),
        ::testing::Values(MathOperation::Inc),
        ::testing::Values(
            CommonGtestArgs::EnqueueSize{32, 64}),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));
