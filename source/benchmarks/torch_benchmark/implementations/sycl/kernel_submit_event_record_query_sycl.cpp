/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_event_record_query.h"
#include "definitions/sycl_kernels.h"

static TestResult run(const KernelSubmitEventRecordQueryArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    Sycl sycl = args.useProfiling
                    ? Sycl{false, sycl::property::queue::enable_profiling()}
                    : Sycl{false};
    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;

    // benchmark
    bool useEvents = false;
    for (size_t i = 0; i < args.iterations; ++i) {
        submit_kernel_empty(wgc, wgs, sycl.queue, useEvents);

        sycl::info::event_command_status status;
        sycl::event event = sycl.queue.ext_oneapi_submit_barrier();
        event.wait();
        // Ensure the event is completed before querying
        status = event.get_info<sycl::info::event::command_execution_status>();
        if (status != sycl::info::event_command_status::complete)
            return TestResult::Error;

        profiler.measureStart();
        for (uint32_t query = 0; query < args.eventQueryIterations; ++query) {
            status = event.get_info<sycl::info::event::command_execution_status>();
        }
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (status != sycl::info::event_command_status::complete)
            return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitEventRecordQuery> registerTestCase(run, Api::SYCL);
