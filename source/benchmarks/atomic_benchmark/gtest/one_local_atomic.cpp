/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/one_local_atomic.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

static const inline RegisterTestCase<OneLocalAtomic> registerTestCase{};

class OneLocalAtomicTest : public ::testing::TestWithParam<std::tuple<DataType, MathOperation, size_t, bool>> {
};

TEST_P(OneLocalAtomicTest, Test) {
    OneLocalAtomicArguments args{};
    args.api = Api::OpenCL;
    args.dataType = std::get<0>(GetParam());
    args.atomicOperation = std::get<1>(GetParam());
    args.workgroupSize = std::get<2>(GetParam());
    args.useEvents = std::get<3>(GetParam());

    OneLocalAtomic test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    OneLocalAtomicTest,
    OneLocalAtomicTest,
    ::testing::Combine(
        ::testing::Values(DataType::Float, DataType::Int32),
        ::CommonGtestArgs::reducedAtomicMathOperations(),
        ::testing::Values(1, 256),
        ::testing::Values(true)));
