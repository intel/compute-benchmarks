/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_memory_reuse.h"
#include "definitions/sycl_kernels.h"

#include <random>

static constexpr size_t REUSE_MEMORY_SIZE = 1024 * 1024 * 64; // 64MB

template <typename data_type>
static TestResult runBenchmark(const KernelSubmitMemoryReuseArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // Setup
    constexpr bool useOoq = false;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}, useOoq};

    data_type *d_reuse = sycl::malloc_device<data_type>(REUSE_MEMORY_SIZE, sycl.queue);
    if (!d_reuse) {
        std::cerr << "Device memory allocation failed" << std::endl;
        return TestResult::Error;
    }
    data_type *d_reuse_end = &d_reuse[REUSE_MEMORY_SIZE - 1];

    // Warmup
    submit_kernel_write<data_type>(sycl.queue, args.useEvents, d_reuse, 0, d_reuse_end, 0);
    sycl.queue.wait();

    // Benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<> offset_dist{0, 10};

    for (size_t i = 0; i < args.iterations; i++) {
        const int offset1 = offset_dist(rng);
        const int offset2 = -offset_dist(rng);

        profiler.measureStart();
        submit_kernel_write<data_type>(sycl.queue, args.useEvents, d_reuse, offset1, d_reuse_end, offset2);
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize == 0)) {
            sycl.queue.wait();
        }
    }
    sycl.queue.wait();

    sycl::free(d_reuse, sycl.queue);

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
