/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_barrier.h"

#include <sycl/sycl.hpp>

static TestResult run(const SubmitBarrierArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun() || (arguments.getLastEvent && !arguments.inOrderQueue)) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    sycl::queue queue{arguments.inOrderQueue ? sycl::property_list{sycl::property::queue::in_order()} : sycl::property_list{}};

    Timer timer;
    const size_t gws = 1u;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    int kernelOperationsCount = 10000;
    const auto eat_time = [=]([[maybe_unused]] auto u) {
        if (kernelOperationsCount > 4) {
            volatile int value = kernelOperationsCount;
            while (value) {
                value = value -1;
            }
        }
    };

    // Warmup
    for (auto iteration = 0u; iteration < 10; iteration++) {
        queue.parallel_for(range, eat_time);

        if (arguments.submitBarrier) {
            queue.ext_oneapi_submit_barrier();
        }

        if (arguments.getLastEvent) {
            queue.ext_oneapi_get_last_event();
        }
    }
    queue.wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (!arguments.useEvents) {
            sycl::ext::oneapi::experimental::nd_launch(queue, range, eat_time);
        } else {
            queue.parallel_for(range, eat_time);
        }

        if (arguments.useHostTasks) {
            queue.submit([&](sycl::handler &cgh) {
                cgh.host_task([=]() { eat_time(0); });
            });
        }

        timer.measureStart();

        if (arguments.submitBarrier) {
            queue.ext_oneapi_submit_barrier();
        }

        if (arguments.getLastEvent) {
            queue.ext_oneapi_get_last_event();
        }

        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    queue.wait();

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitBarrier> registerTestCase(run, Api::SYCL);
