/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_event_record_wait.h"
#include "definitions/sycl_kernels.h"

constexpr int NUM_OF_QUEUES = 2;

static auto enableProfiling = sycl::property::queue::enable_profiling();
static auto inOrder = sycl::property::queue::in_order();

static TestResult run(const KernelSubmitEventRecordWaitArguments &args, Statistics &statistics) {
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

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;

    // benchmark
    bool useEvents = false;
    for (size_t i = 0; i < args.iterations; ++i) {
        submit_kernel_empty(wgc, wgs, q[0], useEvents);

        profiler.measureStart();

        sycl::event e = q[0].ext_oneapi_submit_barrier();
        q[1].ext_oneapi_submit_barrier(std::vector<sycl::event>{e});

        profiler.measureEnd();
        profiler.pushStats(statistics);

        submit_kernel_empty(wgc, wgs, q[1], useEvents);
    }
    for (int i = 0; i < NUM_OF_QUEUES; i++) {
        q[i].wait();
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitEventRecordWait> registerTestCase(run, Api::SYCL);
