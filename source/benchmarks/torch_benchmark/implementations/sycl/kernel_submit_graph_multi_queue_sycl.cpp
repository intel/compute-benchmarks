/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_graph_multi_queue.h"
#include "definitions/sycl_kernels.h"

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
    sycl::queue q[NUM_OF_QUEUES];
    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        q[i] = sycl::queue(dev, args.useProfiling ? sycl::property_list{inOrder, enableProfiling} : sycl::property_list{inOrder});
    }

    const uint32_t wgc = args.workgroupCount;
    const uint32_t wgs = args.workgroupSize;
    const size_t length = wgc * wgs;

    auto make_device_ptr = [&q](size_t size) {
        auto deleter = [queue = q[0]](data_type *ptr) {
            if (ptr)
                sycl::free(ptr, queue);
        };
        using unique_ptr_type = std::unique_ptr<data_type, decltype(deleter)>;
        auto ptr = sycl::malloc_device<data_type>(size, q[0]);
        if (!ptr) {
            throw std::bad_alloc();
        }
        return unique_ptr_type{ptr, deleter};
    };

    auto d_a = make_device_ptr(length);
    auto d_b = make_device_ptr(length);
    auto d_c = make_device_ptr(length);
    auto d_d = make_device_ptr(length);

    // submit kernels
    auto submit_kernels = [&]() {
        data_type add_element = 1.0f;
        // TODO: use enqueue_signal_event when it's available
        sycl::event event1 = q[0].ext_oneapi_submit_barrier();
        // TODO: use enqueue_wait_event when it's available
        q[1].ext_oneapi_submit_barrier({event1});
        submit_kernel_add_const<data_type>(wgc, wgs, q[1], d_c.get(), d_a.get(), add_element);

        sycl::event event2 = q[1].ext_oneapi_submit_barrier();
        submit_kernel_add_const<data_type>(wgc, wgs, q[0], d_b.get(), d_a.get(), add_element);

        q[0].ext_oneapi_submit_barrier({event2});
        submit_kernel_add<data_type>(wgc, wgs, q[0], d_d.get(), d_b.get(), d_c.get());
    };

    // capture graph
    auto graph = syclex::command_graph<syclex::graph_state::modifiable>(q[0].get_context(), q[0].get_device());

    graph.begin_recording(q[0]);
    for (size_t i = 0; i < args.kernelsPerQueue; ++i) {
        submit_kernels();
    }
    graph.end_recording();
    auto graph_exec = graph.finalize();

    // benchmarking
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        // Currently there is an implicit sync inside execute_graph call
        // so only measuring exec + sync is meaningful in terms of comparing
        // perf with L0
        q[0].ext_oneapi_graph(graph_exec);
        q[0].wait();

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphMultiQueue> registerTestCase(run, Api::SYCL);
