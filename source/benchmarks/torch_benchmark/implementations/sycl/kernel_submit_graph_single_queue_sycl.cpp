/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_graph_single_queue.h"

using data_type = float;

namespace syclex = sycl::ext::oneapi::experimental;

static TestResult run(const KernelSubmitGraphSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    bool useOOQ = false;
    Sycl sycl = args.useProfiling
                    ? Sycl{useOOQ, sycl::property::queue::enable_profiling()}
                    : Sycl{useOOQ};
    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const size_t length = wgc * wgs;

    auto d_a = make_device_ptr<data_type>(sycl, length);
    auto d_b = make_device_ptr<data_type>(sycl, length);
    auto d_c = make_device_ptr<data_type>(sycl, length);
    auto d_d = make_device_ptr<data_type>(sycl, length);
    auto d_e = make_device_ptr<data_type>(sycl, length);

    auto submit_kernels = [&]() {
        if (args.kernelName == KernelName::Empty) {
            submit_kernel_empty(wgc, wgs, sycl.queue, args.useEvents);
        } else if (args.kernelName == KernelName::Add) {
            submit_kernel_add<data_type>(wgc, wgs, sycl.queue, args.useEvents, d_a.get(), d_b.get(), d_c.get());
        } else if (args.kernelName == KernelName::AddSequence) {
            data_type add_element_1 = 2.0f;
            data_type add_element_2 = 1.0f;
            submit_kernel_add<data_type>(wgc, wgs, sycl.queue, args.useEvents, d_a.get(), d_b.get(), d_c.get());
            submit_kernel_add_const<data_type>(wgc, wgs, sycl.queue, args.useEvents, d_d.get(), d_a.get(), add_element_1);
            submit_kernel_add_const<data_type>(wgc, wgs, sycl.queue, args.useEvents, d_e.get(), d_d.get(), add_element_2);
        }
    };

    // Capture graph
    auto graph = syclex::command_graph<syclex::graph_state::modifiable>(sycl.queue.get_context(), sycl.queue.get_device());

    graph.begin_recording(sycl.queue);
    for (size_t i = 0; i < args.kernelsPerQueue; ++i) {
        submit_kernels();
    }
    graph.end_recording();
    auto graph_exec = graph.finalize();

    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        for (size_t j = 0; j < args.kernelBatchSize; ++j) {
            // Currently there is an implicit sync inside execute_graph call
            // so only measuring exec + sync is meaningful in terms of comparing
            // perf with L0
            syclex::execute_graph(sycl.queue, graph_exec);
        }
        sycl.queue.wait();

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphSingleQueue> registerTestCase(run, Api::SYCL);
