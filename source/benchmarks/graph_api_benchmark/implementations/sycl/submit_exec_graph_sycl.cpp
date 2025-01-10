/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_exec_graph.h"

#include <gtest/gtest.h>
#include <list>
#include <sycl/ext/oneapi/experimental/graph.hpp>
#include <sycl/sycl.hpp>

using namespace sycl;
namespace sycl_ext = sycl::ext::oneapi::experimental;

constexpr std::size_t N = 1024 * 1024 * 16;

void run_test(queue &Queue, float **Ptr, std::size_t numberOfKernels, bool submit_time, Timer &timer, bool warmup) {

    sycl_ext::command_graph Graph(Queue.get_context(), Queue.get_device());

    if (submit_time && !warmup) {
        timer.measureStart();
    }

    Graph.begin_recording(Queue);

    // prepare data

    event InitEvent = Queue.submit([&](handler &CGH) {
        CGH.parallel_for(range<1>(N), [=](item<1> id) {
            for (std::size_t idx = 0; idx < numberOfKernels; idx++) {
                Ptr[idx][id] = static_cast<float>(idx);
            }
        });
    });

    // submit kernels

    for (std::size_t idx = 0; idx < numberOfKernels; idx++) {

        Queue.submit([&](handler &CGH) {
            CGH.depends_on(InitEvent);
            CGH.parallel_for(range<1>(N), [=](item<1> id) {
                Ptr[idx][id] += 1.0f;
            });
        });
    }

    Graph.end_recording(Queue);
    auto ExecGraph = Graph.finalize();

    Queue.ext_oneapi_graph(ExecGraph);
    // end of submission / start of graph execution

    if (!warmup) {
        if (submit_time) {
            timer.measureEnd();
        } else {
            timer.measureStart();
        }
    }

    Queue.wait_and_throw();

    if (!submit_time && !warmup) {
        timer.measureEnd();
    }
    return;
}

static TestResult run(const SubmitExecGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Timer timer;

    std::size_t numberOfKernels = arguments.numKernels;

    sycl::property_list prop_list{};
    if (arguments.ioq) {
        prop_list = {sycl::property::queue::in_order()};
    }

    queue Queue{sycl::default_selector_v, prop_list};

    if (!Queue.get_device().has(sycl::aspect::ext_oneapi_limited_graph)) {
        return TestResult::DeviceNotCapable;
    }

    if (!Queue.get_device().has(sycl::aspect::usm_shared_allocations) &&
        !Queue.get_device().has(sycl::aspect::usm_device_allocations)) {
        return TestResult::DeviceNotCapable;
    }

    float **Ptr = sycl::malloc_shared<float *>(numberOfKernels, Queue);

    for (std::size_t idx = 0; idx < numberOfKernels; idx++) {
        Ptr[idx] = sycl::malloc_device<float>(N, Queue);
    }

    // prepare graph / warm up
    run_test(Queue, Ptr, numberOfKernels, arguments.measureSubmit, timer, true);

    for (std::size_t itr = 0; itr < arguments.iterations; itr++) {
        run_test(Queue, Ptr, numberOfKernels, arguments.measureSubmit, timer, false);
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (std::size_t idx = 0; idx < numberOfKernels; idx++) {
        sycl::free(Ptr[idx], Queue);
    }
    sycl::free(Ptr, Queue);

    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitExecGraph> registerTestCase(run, Api::SYCL);
