/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};
    Timer timer;
    const size_t gws = 1u;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    const auto empty = [=]([[maybe_unused]] auto i) {};

    // Warmup
    sycl.queue.parallel_for(range, empty).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        sycl.queue.parallel_for(range, empty);

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    sycl.queue.parallel_for(range, empty).wait();

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<ExecuteCommandListImmediate> registerTestCase(run, Api::SYCL);
