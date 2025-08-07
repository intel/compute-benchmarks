/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/record_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<RecordGraph> registerTestCase{};

class RecordGraphTest : public ::testing::TestWithParam<std::tuple<Api, int, int, int, int, int, int, bool, bool, bool, bool>> {
};

TEST_P(RecordGraphTest, Test) {
    RecordGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.nForksInLvl = std::get<1>(GetParam());
    args.nLvls = std::get<2>(GetParam());
    args.nCmdSetsInLvl = std::get<3>(GetParam());
    args.nInstantiations = std::get<4>(GetParam());
    args.nAppendKern = std::get<5>(GetParam());
    args.nAppendCopy = std::get<6>(GetParam());
    args.mRec = std::get<7>(GetParam());
    args.mInst = std::get<8>(GetParam());
    args.mDest = std::get<9>(GetParam());
    args.emulate = std::get<10>(GetParam());
    RecordGraph test;
    test.run(args);
}

INSTANTIATE_TEST_SUITE_P(
    RecordGraphTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0, 1, 2),    // nForksInLvl
        ::testing::Values(1, 4),       // nLvls
        ::testing::Values(1, 10),      // nCmdSetsInLvl
        ::testing::Values(0, 1, 10),   // nInstantiations
        ::testing::Values(0, 1, 10),   // nAppendKern
        ::testing::Values(0, 1, 10),   // nAppendCopy
        ::testing::Values(true),       // mRec
        ::testing::Values(true),       // mInst
        ::testing::Values(false),      // mDest
        ::testing::Values(false, true) // emulate
        ));
