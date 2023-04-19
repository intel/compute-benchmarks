/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/do_math_operation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DoMathOperation> registerTestCase{};

class DoMathOperationTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, CommonGtestArgs::EnqueueSize, bool>> {
};

TEST_P(DoMathOperationTest, Test) {
    DoMathOperationArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.operation = std::get<1>(GetParam());
    args.workgroupCount = std::get<2>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<2>(GetParam()).workgroupSize;
    args.useEvents = std::get<3>(GetParam());

    DoMathOperation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DoMathOperationTest,
    DoMathOperationTest,
    ::testing::Combine(
        ::testing::Values(DataType::Float, DataType::Int32),
        ::testing::ValuesIn(NormalMathOperationArgument::enumValues),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true)));
