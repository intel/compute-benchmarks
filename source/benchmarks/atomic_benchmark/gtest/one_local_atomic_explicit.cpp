/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/one_local_atomic_explicit.h"

#include "framework/enum/test_type.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"
#include "framework/utility/test_type_skip.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<OneLocalAtomicExplicit> registerTestCase{};

class OneLocalAtomicExplicitTest : public ::testing::TestWithParam<std::tuple<Api, DataType, MathOperation, AtomicScope, AtomicMemoryOrder, size_t, bool, TestType>> {
};

TEST_P(OneLocalAtomicExplicitTest, Test) {
    OneLocalAtomicExplicitArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());
    args.atomicOperation = std::get<2>(GetParam());
    args.scope = std::get<3>(GetParam());
    args.memoryOrder = std::get<4>(GetParam());
    args.workgroupSize = std::get<5>(GetParam());
    args.useEvents = std::get<6>(GetParam());

    const auto testType = std::get<7>(GetParam());
    if (isTestSkipped(Configuration::get().extended, testType)) {
        GTEST_SKIP();
    }

    OneLocalAtomicExplicit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    OneLocalAtomicExplicitTest,
    OneLocalAtomicExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::reducedAtomicMathOperations(),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::testing::Values(1, 256),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));

INSTANTIATE_TEST_SUITE_P(
    OneLocalAtomicExplicitExtendedTest,
    OneLocalAtomicExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::testing::Values(1, 64, 256),
        ::testing::Values(true),
        ::testing::Values(TestType::Extended)));

INSTANTIATE_TEST_SUITE_P(
    OneLocalAtomicExplicitTestLIMITED,
    OneLocalAtomicExplicitTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Int32),
        ::testing::Values(MathOperation::Inc),
        ::testing::Values(AtomicScope::Device),
        ::testing::Values(AtomicMemoryOrder::Relaxed),
        ::testing::Values(1, 256),
        ::testing::Values(true),
        ::testing::Values(TestType::Regular)));
