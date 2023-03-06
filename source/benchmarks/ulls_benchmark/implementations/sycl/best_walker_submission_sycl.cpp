/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_submission.h"

static TestResult run(const BestWalkerSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::gpu_selector{}};
    Timer timer;
    auto size = 1u;

    // Create buffer
    auto buffer = sycl::malloc_host<uint64_t>(size, sycl.queue);
    volatile auto volatileBuffer = buffer;

    // Create kernel
    const auto writeOne = [buffer](auto i) {
        buffer[0] = 1u;
    };

    // Warmup
    sycl.queue.parallel_for(size, writeOne).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *buffer = 0u;

        timer.measureStart();

        sycl.queue.parallel_for(size, writeOne);
        while (*volatileBuffer != 1u) {
        }

        timer.measureEnd();

        sycl.queue.wait();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    sycl::free(buffer, sycl.queue);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::SYCL);