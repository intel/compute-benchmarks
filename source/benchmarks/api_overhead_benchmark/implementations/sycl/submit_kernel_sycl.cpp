/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>
#include <sycl/sycl.hpp>

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

static const sycl::property_list queueProps[] = {
    sycl::property_list{},
    sycl::property_list{enableProfiling},
    sycl::property_list{inOrder},
    sycl::property_list{inOrder, enableProfiling},
};

static TestResult run(const SubmitKernelArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    auto queuePropsIndex = 0;
    queuePropsIndex |= arguments.useProfiling ? 0x1 : 0;
    queuePropsIndex |= arguments.inOrderQueue ? 0x2 : 0;
    sycl::queue queue{queueProps[queuePropsIndex]};

    const size_t gws = 1u;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    // Create kernel
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);
    const auto eat_time = [=]([[maybe_unused]] auto u) {
        if (kernelOperationsCount > 4) {
            volatile int value = kernelOperationsCount;
            while (value > 1)
                value -= 1;
        }
    };

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        prof.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
            if (!arguments.useEvents) {
#ifndef __ACPP__
                sycl::ext::oneapi::experimental::nd_launch(queue, range, eat_time);
#else
                // AdaptiveCpp does not support oneAPI extensions.
                // The "coarse-grained events" extension may be similar.
                queue.parallel_for(range, eat_time);
#endif
            } else {
                queue.parallel_for(range, eat_time);
            }
        }

        if (!arguments.measureCompletionTime) {
            prof.measureEnd();
        }

        queue.wait();

        if (arguments.measureCompletionTime) {
            prof.measureEnd();
        }

        prof.pushStats(statistics);
    }
    queue.wait();

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel> registerTestCase(run, Api::SYCL);
