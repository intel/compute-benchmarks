/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/mutate_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<MutateGraph> registerTestCase{};

class MutateGraphTest : public ::testing::TestWithParam<std::tuple<bool, uint32_t, uint32_t, bool>> {
};

TEST_P(MutateGraphTest, Creation) {
    MutateGraphArguments args{};
    args.api = Api::L0;
    args.operationType = GraphOperationType::Init;
    args.useInOrder = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());
    args.changeRate = std::get<2>(GetParam());
    args.canUpdate = std::get<3>(GetParam());

    MutateGraph test;
    test.run(args);
}

TEST_P(MutateGraphTest, Mutation) {
    MutateGraphArguments args{};
    args.api = Api::L0;
    args.operationType = GraphOperationType::Mutate;
    args.useInOrder = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());
    args.changeRate = std::get<2>(GetParam());
    args.canUpdate = std::get<3>(GetParam());

    MutateGraph test;
    test.run(args);
}

TEST_P(MutateGraphTest, Execution) {
    MutateGraphArguments args{};
    args.api = Api::L0;
    args.operationType = GraphOperationType::Execute;
    args.useInOrder = std::get<0>(GetParam());
    args.numKernels = std::get<1>(GetParam());
    args.changeRate = std::get<2>(GetParam());
    args.canUpdate = std::get<3>(GetParam());

    MutateGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    MutateGraphTest,
    MutateGraphTest,
    ::testing::Combine(
        ::testing::Values(false, true),
        ::testing::Values(100, 200, 500),
        ::testing::Values(1, 5, 10),
        ::testing::Values(false, true)));
