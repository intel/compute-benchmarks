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

class SeparateAtomicsExplicitTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, size_t, AtomicScope, AtomicMemoryOrder, CommonGtestArgs::EnqueueSize, bool, TestType>> {
};

TEST_P(SeparateAtomicsExplicitTest, Test) {
    SeparateAtomicsExplicitArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.atomicOperation = std::get<1>(GetParam());
    args.atomicsPerCacheline = std::get<2>(GetParam());
    args.scope = std::get<3>(GetParam());
    args.memoryOrder = std::get<4>(GetParam());
    args.workgroupCount = std::get<5>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<5>(GetParam()).workgroupSize;
    args.useEvents = std::get<6>(GetParam());

    if (args.atomicsPerCacheline > args.workgroupCount * args.workgroupSize) {
        GTEST_SKIP();
    }

    const auto testType = std::get<7>(GetParam());
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
    ::testing::ValuesIn([] {
        std::vector<std::tuple<DataType, MathOperation, size_t, AtomicScope, AtomicMemoryOrder, CommonGtestArgs::EnqueueSize, bool, TestType>> testCases;
        testCases.emplace_back(DataType::Int32, MathOperation::Add, 1, AtomicScope::Workgroup, AtomicMemoryOrder::AcquireRelease, CommonGtestArgs::EnqueueSize{32, 64}, true, TestType::Regular);
        testCases.emplace_back(DataType::Int32, MathOperation::Add, 1, AtomicScope::Workgroup, AtomicMemoryOrder::SequentialConsitent, CommonGtestArgs::EnqueueSize{32, 64}, true, TestType::Regular);
        testCases.emplace_back(DataType::Int32, MathOperation::Add, 4, AtomicScope::Device, AtomicMemoryOrder::AcquireRelease, CommonGtestArgs::EnqueueSize{32, 64}, true, TestType::Regular);
        return testCases;
    }()));
