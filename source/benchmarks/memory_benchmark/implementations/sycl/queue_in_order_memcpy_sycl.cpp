/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/queue_in_order_memcpy.h"

static TestResult run(const QueueInOrderMemcpyArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);

    if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    constexpr bool useOOQ = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOOQ};

    // Create buffers
    constexpr size_t alignment = 512;
    void *source{}, *destination{};
    auto allocateMemory = [&](UsmMemoryPlacement placement) {
        if (placement == UsmMemoryPlacement::Device) {
            return UsmHelper::allocateAligned(placement, sycl, alignment, arguments.size);
        } else {
            return UsmHelper::allocate(placement, sycl, arguments.size);
        }
    };
    source = allocateMemory(arguments.sourcePlacement);
    destination = allocateMemory(arguments.destinationPlacement);

    // Warmup
    for (auto j = 0u; j < arguments.count; ++j) {
        sycl.queue.memcpy(destination, source, arguments.size);
    }
    sycl.queue.wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        prof.measureStart();
        for (auto j = 0u; j < arguments.count; ++j) {
            sycl.queue.memcpy(destination, source, arguments.size);
        }
        sycl.queue.wait();
        prof.measureEnd();
        prof.pushStats(statistics);
    }

    // Cleanup
    UsmHelper::deallocate(arguments.sourcePlacement, sycl, source);
    UsmHelper::deallocate(arguments.destinationPlacement, sycl, destination);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<QueueInOrderMemcpy> registerTestCase(run, Api::SYCL);
