/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/one_atomic_explicit.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<OneAtomicExplicit> registerTestCase{};

class OneAtomicExplicitTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, AtomicScope, AtomicMemoryOrder, CommonGtestArgs::EnqueueSize, bool>> {
};

TEST_P(OneAtomicExplicitTest, Test) {
    OneAtomicExplicitArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.atomicOperation = std::get<1>(GetParam());
    args.scope = std::get<2>(GetParam());
    args.memoryOrder = std::get<3>(GetParam());
    args.workgroupCount = std::get<4>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<4>(GetParam()).workgroupSize;
    args.useEvents = std::get<5>(GetParam());

    OneAtomicExplicit test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    OneAtomicExplicitTest,
    OneAtomicExplicitTest,
    ::testing::Combine(
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::testing::ValuesIn(AtomicScopeHelper::allValues),
        ::testing::ValuesIn(AtomicMemoryOrderHelper::allValues),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true)));
