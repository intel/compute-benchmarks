/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/queue_memcpy.h"

static TestResult run(const QueueMemcpyArguments &arguments, Statistics &statistics) {
    // Setup
    Sycl sycl{sycl::gpu_selector{}};
    Timer timer;

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
    sycl.queue.memcpy(destination, source, arguments.size);
    sycl.queue.wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        sycl.queue.memcpy(destination, source, arguments.size).wait();
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    UsmHelper::deallocate(arguments.sourcePlacement, sycl, source);
    UsmHelper::deallocate(arguments.destinationPlacement, sycl, destination);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueueMemcpy> registerTestCase(run, Api::SYCL);
