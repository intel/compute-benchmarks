/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/empty_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const EmptyKernelArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};
    const size_t gws = arguments.workgroupCount * arguments.workgroupSize;
    const size_t lws = arguments.workgroupSize;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    const auto empty = [=]([[maybe_unused]] auto i) {};

    // Warmup
    sycl.queue.parallel_for(range, empty).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        prof.measureStart();

        sycl.queue.parallel_for(range, empty).wait();

        prof.measureEnd();
        prof.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<EmptyKernel> registerTestCase(run, Api::SYCL);
