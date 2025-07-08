/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/finalize_graph.h"

#include "framework/enum/graph_structure.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<FinalizeGraph> registerTestCase{};

class FinalizeGraphTest : public ::testing::TestWithParam<std::tuple<Api, bool, GraphStructure>> {
};

TEST_P(FinalizeGraphTest, Test) {
    FinalizeGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.rebuildGraphEveryIter = std::get<1>(GetParam());
    args.graphStructure = std::get<2>(GetParam());
    FinalizeGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    FinalizeGraphTest,
    FinalizeGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL),
        ::testing::Values(true, false),                                                        // firstFinalizeOnly
        ::testing::Values(GraphStructure::Gromacs, GraphStructure::LLama, GraphStructure::Amr) // graphStructure
        ));
