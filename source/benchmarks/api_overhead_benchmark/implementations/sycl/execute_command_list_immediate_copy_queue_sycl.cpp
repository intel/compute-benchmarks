/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateCopyQueueArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};

    // Create buffers
    auto srcBuffer = UsmHelper::allocate(arguments.sourcePlacement, sycl, arguments.size);
    auto dstBuffer = UsmHelper::allocate(arguments.destinationPlacement, sycl, arguments.size);

    // Warmup
    sycl.queue.memcpy(dstBuffer, srcBuffer, arguments.size).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        prof.measureStart();
        sycl.queue.memcpy(dstBuffer, srcBuffer, arguments.size);

        if (!arguments.measureCompletionTime) {
            prof.measureEnd();
        }

        sycl.queue.wait();

        if (arguments.measureCompletionTime) {
            prof.measureEnd();
        }

        prof.pushStats(statistics);
    }

    // Cleanup
    UsmHelper::deallocate(arguments.sourcePlacement, sycl, srcBuffer);
    UsmHelper::deallocate(arguments.destinationPlacement, sycl, dstBuffer);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<ExecuteCommandListImmediateCopyQueue> registerTestCase(run, Api::SYCL);
