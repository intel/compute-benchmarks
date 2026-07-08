/*
 * Copyright (C) 2025-2026 Intel Corporation
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

// Each suite below, except RecordGraphLargeTest, sweeps exactly one
// graph-shape dimension while holding the other five
// (fork/lvl/cmdSet/instantiation/appendKern/appendCopy) at a baseline of 1
// -- except nLvls in the forks suite, which is 2, since forks require at
// least one nesting level to have any effect. RecordGraphLargeTest instead
// sets all six dimensions to their prior maximum values.
INSTANTIATE_TEST_SUITE_P(
    RecordGraphForksTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(0, 1, 2), // nForksInLvl
        ::testing::Values(2),       // nLvls (>1, otherwise forks never nest and this sweep is a no-op)
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(true),       // mRec
        ::testing::Values(true),       // mInst
        ::testing::Values(false),      // mDest
        ::testing::Values(false, true) // emulate
        ));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphLevelsTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1),
        ::testing::Values(1, 4), // nLvls
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphCmdSetsTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1, 10), // nCmdSetsInLvl
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphInstantiationsTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(0, 1, 10), // nInstantiations
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphAppendKernTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(0, 1, 10), // nAppendKern
        ::testing::Values(1),
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphAppendCopyTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(1),
        ::testing::Values(0, 1, 10), // nAppendCopy
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));

INSTANTIATE_TEST_SUITE_P(
    RecordGraphLargeTest,
    RecordGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(2),  // nForksInLvl
        ::testing::Values(4),  // nLvls
        ::testing::Values(10), // nCmdSetsInLvl
        ::testing::Values(10), // nInstantiations
        ::testing::Values(10), // nAppendKern
        ::testing::Values(10), // nAppendCopy
        ::testing::Values(true),
        ::testing::Values(true),
        ::testing::Values(false),
        ::testing::Values(false, true)));
