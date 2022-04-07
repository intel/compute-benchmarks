/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include "definitions/separate_atomic.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<SeparateAtomics> registerTestCase{};

class SeparateAtomicsTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, size_t, CommonGtestArgs::EnqueueSize>> {
};

TEST_P(SeparateAtomicsTest, Test) {
    SeparateAtomicsArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.atomicOperation = std::get<1>(GetParam());
    args.atomicsPerCacheline = std::get<2>(GetParam());
    args.workgroupCount = std::get<3>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<3>(GetParam()).workgroupSize;

    if (args.atomicsPerCacheline > args.workgroupCount * args.workgroupSize) {
        GTEST_SKIP();
    }

    SeparateAtomics test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    SeparateAtomicsTest,
    SeparateAtomicsTest,
    ::testing::Combine(
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::allAtomicMathOperations(),
        ::testing::Values(1, 4),
        ::CommonGtestArgs::enqueueSizesForAtomics()));
