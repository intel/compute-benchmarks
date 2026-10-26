/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/do_math_operation.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<DoMathOperation> registerTestCase{};

class DoMathOperationTest : public ::testing::TestWithParam<std::tuple<Api, DataType, MathOperation, CommonGtestArgs::EnqueueSize, bool, bool>> {
};

TEST_P(DoMathOperationTest, Test) {
    DoMathOperationArguments args{};
    args.api = std::get<0>(GetParam());
    args.dataType = std::get<1>(GetParam());
    args.operation = std::get<2>(GetParam());
    args.workgroupCount = std::get<3>(GetParam()).workgroupCount;
    args.workgroupSize = std::get<3>(GetParam()).workgroupSize;
    args.useEvents = std::get<4>(GetParam());
    args.mixGrfModes = std::get<5>(GetParam());

    DoMathOperation test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    DoMathOperationTest,
    DoMathOperationTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Float),
        ::testing::ValuesIn(NormalMathOperationArgument::enumValues),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    DoMathOperationTestInt32,
    DoMathOperationTest,
    ::testing::Combine(
        ::testing::Values(Api::OpenCL, Api::L0),
        ::testing::Values(DataType::Int32),
        ::testing::Values(MathOperation::Add, MathOperation::Sub, MathOperation::Div, MathOperation::Modulo, MathOperation::Min,
                          MathOperation::Max, MathOperation::And, MathOperation::Or, MathOperation::Xor),
        ::CommonGtestArgs::enqueueSizesForAtomics(),
        ::testing::Values(true),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(
    DoMathOperationTestLIMITED,
    DoMathOperationTest,
    ::testing::ValuesIn([] {
        std::vector<std::tuple<Api, DataType, MathOperation, CommonGtestArgs::EnqueueSize, bool, bool>> testCases;
        for (const Api api : {Api::OpenCL, Api::L0}) {
            testCases.push_back(std::make_tuple(api, DataType::Float, MathOperation::Add, CommonGtestArgs::EnqueueSize{32, 64}, true, false));
            testCases.push_back(std::make_tuple(api, DataType::Int32, MathOperation::Max, CommonGtestArgs::EnqueueSize{32, 64}, true, false));
            testCases.push_back(std::make_tuple(api, DataType::Int32, MathOperation::Modulo, CommonGtestArgs::EnqueueSize{32, 64}, true, false));
        }
        return testCases;
    }()));
