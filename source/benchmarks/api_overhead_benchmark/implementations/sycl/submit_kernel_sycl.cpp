/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();
static auto discardEvents = sycl::ext::oneapi::property::queue::discard_events();

static const sycl::property_list queueProps[] = {
    sycl::property_list{},
    sycl::property_list{enableProfiling},
    sycl::property_list{inOrder},
    sycl::property_list{inOrder, enableProfiling},
    // Note: discard_events cannot be used with enable_profiling!
    // Note: empirically, discard_events also appears to be ignored for
    // out-of-order queues, but this combination does not generate an error, so
    // we will keep it.
    sycl::property_list{discardEvents},
    sycl::property_list{enableProfiling},
    sycl::property_list{discardEvents, inOrder},
    sycl::property_list{inOrder, enableProfiling},
};

static TestResult run(const SubmitKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    auto queuePropsIndex = 0;
    queuePropsIndex |= arguments.useProfiling ? 0x1 : 0;
    queuePropsIndex |= arguments.inOrderQueue ? 0x2 : 0;
    queuePropsIndex |= arguments.discardEvents ? 0x4 : 0;
    sycl::queue queue{queueProps[queuePropsIndex]};

    Timer timer;
    const size_t gws = 1u;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);
    const auto eat_time = [=]([[maybe_unused]] auto u) {
        volatile int value = 1u;
        for (int i = 0; i < kernelOperationsCount; i++) {
            value /= 2;
            value *= 2;
        }
    };

    // Warmup
    for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
        queue.parallel_for(range, eat_time);
    }
    queue.wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
            queue.parallel_for(range, eat_time);
        }

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        queue.wait();

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }
    queue.wait();

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel> registerTestCase(run, Api::SYCL);
