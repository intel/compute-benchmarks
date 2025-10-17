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

#include "definitions/separate_atomic_explicit.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SeparateAtomicsExplicit> registerTestCase{};

class SeparateAtomicsExplicitTest : public ::testing::TestWithParam<std::tuple<Api, DataType, MathOperation, size_t, AtomicScope, AtomicMemoryOrder, CommonGtestArgs::EnqueueSize, bool, TestType>> {
};

TEST_P(SeparateAtomicsExplicitTest, Test) {
    SeparateAtomicsExplicitArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());
    args.atomicOperation = std::get<2>(GetParam());
    args.atomicsPerCacheline = std::get<3>(GetParam());
    args.scope = std::get<4>(GetParam());
    args.memoryOrder = std::get<5>(GetParam());
    args.workgroupCount = std::get<6>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<6>(GetParam()).workgroupSize;
    args.useEvents = std::get<7>(GetParam());

    if (args.atomicsPerCacheline > args.workgroupCount * args.workgroupSize) {
        GTEST_SKIP();
    }

    const auto testType = std::get<8>(GetParam());
    if (isTestSkipped(Configuration::get().extended, testType)) {
        GTEST_SKIP();
    }

    SeparateAtomicsExplicit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsExplicitTest,
    SeparateAtomicsExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::reducedAtomicMathOperations(),
        ::testing::Values(1, 4),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::CommonGtestArgs::reducedEnqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsExplicitExtendedTest,
    SeparateAtomicsExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::testing::Values(1, 4),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(TestType::Extended)));

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsExplicitTestLIMITED,
    SeparateAtomicsExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Int32),
        ::testing::Values(MathOperation::Add),
        ::testing::Values(1, 4),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::Values(AtomicMemoryOrder::AcquireRelease, AtomicMemoryOrder::SequentialConsitent),
        ::testing::Values(CommonGtestArgs::EnqueueSize{32, 64}),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));
