/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/execute_command_list_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};
    const size_t gws = 1u;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    const auto empty = [=]([[maybe_unused]] auto i) {};

    // Warmup
    auto event = sycl.queue.parallel_for(range, empty);
    if (arguments.useEventForHostSync) {
        event.wait();
    } else {
        sycl.queue.wait();
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        prof.measureStart();
        for (auto iteration = 0u; iteration < arguments.amountOfCalls; iteration++) {
            if (arguments.useEventForHostSync) {
                event = sycl.queue.parallel_for(range, event, empty);
            } else {
                sycl.queue.parallel_for(range, empty);
            }
        }

        if (arguments.measureCompletionTime) {
            if (arguments.useEventForHostSync) {
                event.wait();
            } else {
                sycl.queue.wait();
            }
        }
        prof.measureEnd();
        prof.pushStats(statistics);
    }
    sycl.queue.wait();

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<ExecuteCommandListImmediate> registerTestCase(run, Api::SYCL);
