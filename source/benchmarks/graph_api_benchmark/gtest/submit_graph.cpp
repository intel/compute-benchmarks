/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/submit_graph.h"

#include "framework/test_case/register_test_case.h"
#include "framework/utility/common_gtest_args.h"

#include <gtest/gtest.h>

[[maybe_unused]] static const inline RegisterTestCase<SubmitGraph> registerTestCase{};

class SubmitGraphTest : public ::testing::TestWithParam<std::tuple<Api, bool, bool, bool, bool, bool, bool, size_t, size_t, bool>> {
};

TEST_P(SubmitGraphTest, Test) {
    SubmitGraphArguments args{};
    args.api = std::get<0>(GetParam());
    args.useProfiling = std::get<1>(GetParam());
    args.useHostTasks = std::get<2>(GetParam());
    args.inOrderQueue = std::get<3>(GetParam());
    args.useEvents = std::get<4>(GetParam());
    args.useExplicit = std::get<5>(GetParam());
    args.emulateGraphs = std::get<6>(GetParam());
    args.numKernels = std::get<7>(GetParam());
    args.kernelExecutionTime = std::get<8>(GetParam());
    args.measureCompletionTime = std::get<9>(GetParam());
    SubmitGraph test;
    test.run(args);
}

// The flag space is backend-specific, so split it into per-backend suites that
// only generate combinations each API can actually run (avoids ApiNotCapable):
//  - useHostTasks and useExplicit are SYCL-only.
//  - emulateGraphs selects the backend: command-buffer/emulation (L0, OpenCL,
//    UR) vs native graph record & replay (L0, SYCL, UR).
//  - UR's native graph mode requires an in-order queue.

// Command-buffer / emulation mode (emulateGraphs = true).
INSTANTIATE_TEST_SUITE_P(
    SubmitGraphCmdBuffer,
    SubmitGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0, Api::OpenCL, Api::UR),
        ::testing::Values(false, true),   // useProfiling
        ::testing::Values(false),         // useHostTasks (SYCL-only)
        ::testing::Values(false, true),   // inOrderQueue
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(false),         // useExplicit (SYCL-only)
        ::testing::Values(true),          // emulateGraphs
        ::testing::Values(4u, 32u),       // numKernels
        ::testing::Values(1u),            // kernelExecutionTime
        ::testing::Values(false, true))); // measureCompletionTime

// Native graph record & replay (emulateGraphs = false). Level Zero supports
// both queue orderings.
INSTANTIATE_TEST_SUITE_P(
    SubmitGraphNativeGraph,
    SubmitGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::L0),
        ::testing::Values(false, true),   // useProfiling
        ::testing::Values(false),         // useHostTasks (SYCL-only)
        ::testing::Values(false, true),   // inOrderQueue
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(false),         // useExplicit (SYCL-only)
        ::testing::Values(false),         // emulateGraphs
        ::testing::Values(4u, 32u),       // numKernels
        ::testing::Values(1u),            // kernelExecutionTime
        ::testing::Values(false, true))); // measureCompletionTime

// UR native graph record & replay requires an in-order queue.
INSTANTIATE_TEST_SUITE_P(
    SubmitGraphNativeGraphUr,
    SubmitGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::UR),
        ::testing::Values(false, true),   // useProfiling
        ::testing::Values(false),         // useHostTasks (SYCL-only)
        ::testing::Values(true),          // inOrderQueue (required by UR)
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(false),         // useExplicit (SYCL-only)
        ::testing::Values(false),         // emulateGraphs
        ::testing::Values(4u, 32u),       // numKernels
        ::testing::Values(1u),            // kernelExecutionTime
        ::testing::Values(false, true))); // measureCompletionTime

// SYCL native graph, exercising the SYCL-only host-task and explicit-graph paths.
INSTANTIATE_TEST_SUITE_P(
    SubmitGraphSycl,
    SubmitGraphTest,
    ::testing::Combine(
        ::testing::Values(Api::SYCL),
        ::testing::Values(false, true),   // useProfiling
        ::testing::Values(false, true),   // useHostTasks
        ::testing::Values(false, true),   // inOrderQueue
        ::testing::Values(false, true),   // useEvents
        ::testing::Values(false, true),   // useExplicit
        ::testing::Values(false),         // emulateGraphs
        ::testing::Values(4u, 32u),       // numKernels
        ::testing::Values(1u),            // kernelExecutionTime
        ::testing::Values(false, true))); // measureCompletionTime
