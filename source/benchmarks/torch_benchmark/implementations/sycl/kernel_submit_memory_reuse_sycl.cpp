/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_memory_reuse.h"

#include <random>

static constexpr size_t REUSE_MEMORY_SIZE = 1024 * 1024 * 64; // 64MB

template <typename data_type>
static TestResult runBenchmark(const KernelSubmitMemoryReuseArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // Setup
    constexpr bool useOoq = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOoq};

    auto d_reuse = make_device_ptr<data_type>(sycl, REUSE_MEMORY_SIZE);
    data_type *d_reuse_end = &d_reuse.get()[REUSE_MEMORY_SIZE - 1];

    // Benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<> offset_dist{0, 10};

    for (size_t i = 0; i < args.iterations; i++) {
        const int offset1 = offset_dist(rng);
        const int offset2 = -offset_dist(rng);

        profiler.measureStart();
        submit_kernel_write<data_type>(sycl.queue, args.useEvents, d_reuse.get(), offset1, d_reuse_end, offset2);
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize == 0)) {
            sycl.queue.wait();
        }
    }
    sycl.queue.wait();

    return TestResult::Success;
}

static TestResult run(const KernelSubmitMemoryReuseArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    switch (args.kernelDataType) {
    case DataType::Int32:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<int>(args, profiler, statistics));
        break;
    case DataType::Float:
        ASSERT_TEST_RESULT_SUCCESS(runBenchmark<float>(args, profiler, statistics));
        break;
    default:
        return TestResult::InvalidArgs;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMemoryReuse> registerTestCase(run, Api::SYCL);
