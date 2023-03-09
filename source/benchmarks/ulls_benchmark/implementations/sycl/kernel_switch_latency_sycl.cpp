/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Timer timer;
    auto size = 1u;
    Sycl sycl{sycl::property::queue::enable_profiling{}};

    // Create buffer
    auto buffer = sycl::malloc_host<uint32_t>(size, sycl.queue);

    // Create kernel
    const auto kernel = [buffer]() {
        *buffer = 1u;
    };

    std::vector<sycl::event> events(arguments.kernelCount);

    // Warmup
    sycl.queue.single_task(kernel).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        events[0] = sycl.queue.single_task(kernel);
        for (auto j = 1u; j < arguments.kernelCount; j++) {
            events[j] = sycl.queue.single_task(events[j - 1], kernel);
        }
        events.back().wait();
        timer.measureEnd();

        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto j = 1u; j < arguments.kernelCount; j++) {
            auto end = events[j - 1].get_profiling_info<sycl::info::event_profiling::command_end>();
            auto start = events[j].get_profiling_info<sycl::info::event_profiling::command_start>();
            switchTime += std::chrono::nanoseconds(start - end);
        }
        statistics.pushValue(switchTime / (arguments.kernelCount - 1), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSwitchLatency> registerTestCase(run, Api::SYCL);
