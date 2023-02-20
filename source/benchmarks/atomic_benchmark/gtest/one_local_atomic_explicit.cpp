/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/one_local_atomic_explicit.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<OneLocalAtomicExplicit> registerTestCase{};

class OneLocalAtomicExplicitTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, AtomicScope, AtomicMemoryOrder, size_t, bool>> {
};

TEST_P(OneLocalAtomicExplicitTest, Test) {
    OneLocalAtomicExplicitArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.atomicOperation = std::get<1>(GetParam());
    args.scope = std::get<2>(GetParam());
    args.memoryOrder = std::get<3>(GetParam());
    args.workgroupSize = std::get<4>(GetParam());
    args.useEvents = std::get<5>(GetParam());

    OneLocalAtomicExplicit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    OneLocalAtomicExplicitTest,
    OneLocalAtomicExplicitTest,
    ::testing::Combine(
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::reducedAtomicMathOperations(),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::testing::Values(1, 256),
        ::testing::Values(true)));
