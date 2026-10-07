/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_multi_queue.h"

using data_type = int;

constexpr int NUM_OF_QUEUES = 2;

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

static TestResult run(const KernelSubmitMultiQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    sycl::device dev = sycl::device(sycl::gpu_selector_v);
    Sycl sycl[NUM_OF_QUEUES] = {
        args.useProfiling ? Sycl{dev, inOrder, enableProfiling} : Sycl{dev, inOrder},
        args.useProfiling ? Sycl{dev, inOrder, enableProfiling} : Sycl{dev, inOrder}};

    const size_t data_size = args.kernelWGCount * args.kernelWGSize;

    using DevicePtr = decltype(make_device_ptr<data_type>(sycl[0], data_size));
    std::vector<DevicePtr> d_a;
    std::vector<DevicePtr> d_b;
    std::vector<DevicePtr> d_c;

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        d_a.push_back(make_device_ptr<data_type>(sycl[i], data_size));
        d_b.push_back(make_device_ptr<data_type>(sycl[i], data_size));
        d_c.push_back(make_device_ptr<data_type>(sycl[i], data_size));
    }

    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        sycl[i].queue.wait();
    }

    for (size_t i = 0; i < args.iterations; i++) {

        if (args.measureCompletionTime) {
            profiler.measureStart();
        }

        for (size_t j = 0; j < args.kernelsPerQueue; j++) {
            submit_kernel_add<data_type>(args.kernelWGCount, args.kernelWGSize, sycl[0].queue, args.useEvents, d_a[0].get(), d_b[0].get(), d_c[0].get());
        }

        for (size_t j = 1; j < args.kernelsPerQueue; j++) {
            submit_kernel_add<data_type>(args.kernelWGCount, args.kernelWGSize, sycl[1].queue, args.useEvents, d_a[1].get(), d_b[1].get(), d_c[1].get());
        }
        sycl::event q2_last_event = submit_with_event_kernel_add<data_type>(args.kernelWGCount, args.kernelWGSize, sycl[1].queue, args.useEvents, d_a[1].get(), d_b[1].get(), d_c[1].get());

        if (!args.measureCompletionTime) {
            profiler.measureStart();
        }
        submit_kernel_add<data_type>(args.kernelWGCount, args.kernelWGSize, sycl[0].queue, args.useEvents, q2_last_event, d_a[0].get(), d_b[0].get(), d_c[0].get());
        if (!args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }

        for (int i = 0; i < NUM_OF_QUEUES; i++) {
            sycl[i].queue.wait();
        }

        if (args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMultiQueue> registerTestCase(run, Api::SYCL);
