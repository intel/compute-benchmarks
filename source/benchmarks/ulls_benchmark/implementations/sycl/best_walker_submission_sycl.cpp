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

#include "definitions/best_walker_submission.h"

static TestResult run(const BestWalkerSubmissionArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};
    auto size = 1u;

    // Create buffer
    auto buffer = sycl::malloc_host<uint32_t>(size, sycl.queue);
    volatile auto volatileBuffer = buffer;

    // Create kernel
    const auto writeOne = [buffer]() {
        *buffer = 1u;
    };

    // Warmup
    sycl.queue.single_task(writeOne).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *buffer = 0u;

        prof.measureStart();

        sycl.queue.single_task(writeOne);
        while (*volatileBuffer != 1u) {
        }

        prof.measureEnd();
        prof.pushStats(statistics);

        sycl.queue.wait();
    }

    // Cleanup
    sycl::free(buffer, sycl.queue);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::SYCL);
