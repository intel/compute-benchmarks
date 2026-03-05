/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_graph_multi_queue.h"

using data_type = float;

constexpr int NUM_OF_QUEUES = 2;

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

namespace syclex = sycl::ext::oneapi::experimental;

static TestResult run(const KernelSubmitGraphMultiQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    sycl::device dev = sycl::device(sycl::gpu_selector_v);
    Sycl sycl[NUM_OF_QUEUES];
    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        if (args.useProfiling) {
            sycl[i] = Sycl(dev, inOrder, enableProfiling);
        } else {
            sycl[i] = Sycl(dev, inOrder);
        }
    }

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const size_t length = wgc * wgs;

    auto d_a = make_device_ptr<data_type>(sycl[0], length);
    auto d_b = make_device_ptr<data_type>(sycl[0], length);
    auto d_c = make_device_ptr<data_type>(sycl[0], length);
    auto d_d = make_device_ptr<data_type>(sycl[0], length);

    // submit kernels
    auto submit_kernels = [&]() {
        data_type add_element = 1.0f;

        sycl::event event1 = submit_with_event_kernel_add_const<data_type>(
            wgc, wgs, sycl[0].queue, d_b.get(), d_a.get(), add_element);

        sycl::event event2 = submit_with_event_kernel_add_const<data_type>(
            wgc, wgs, sycl[1].queue, event1, d_c.get(), d_b.get(), add_element);

        submit_kernel_add<data_type>(wgc, wgs, sycl[0].queue, args.useEvents, event2, d_d.get(), d_b.get(), d_c.get());
    };

    // capture graph
    auto graph = syclex::command_graph<syclex::graph_state::modifiable>(sycl[0].queue.get_context(), sycl[0].queue.get_device());

    graph.begin_recording(sycl[0].queue);
    for (size_t i = 0; i < args.kernelsPerQueue; ++i) {
        submit_kernels();
    }
    graph.end_recording();
    auto graph_exec = graph.finalize();

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        // Currently there is an implicit sync inside execute_graph call
        // so only measuring exec + sync is meaningful in terms of comparing
        // perf with L0
        sycl[0].queue.ext_oneapi_graph(graph_exec);
        sycl[0].queue.wait();

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphMultiQueue> registerTestCase(run, Api::SYCL);
