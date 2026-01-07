/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_memory_reuse.h"
#include "kernel_submit_common.hpp"

#include <random>

static constexpr size_t REUSE_MEMORY_SIZE = 1024 * 1024 * 64; // 64MB

template <typename data_type>
static TestResult launch_kernel_l0(ze_command_list_handle_t cmdListImmediate,
                                   ze_kernel_handle_t kernel,
                                   ze_group_count_t dispatch,
                                   data_type *ptr1,
                                   int offset_1,
                                   data_type *ptr2,
                                   int offset_2) {
    void *kernelArguments[4] = {&ptr1, &offset_1, &ptr2, &offset_2};
    ze_group_size_t groupSizes = {1u, 1u, 1u};

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate, kernel, dispatch, groupSizes,
                                                                          kernelArguments, nullptr, nullptr, 0, nullptr));
    return TestResult::Success;
}

template <typename data_type>
static TestResult runBenchmark(const KernelSubmitMemoryReuseArguments &args, ComboProfilerWithStats &profiler, Statistics &statistics) {
    // Setup
    L0Context ctx{};

    data_type *d_reuse = nullptr;
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(ctx.l0, REUSE_MEMORY_SIZE, d_reuse));
    data_type *d_reuse_end = &d_reuse[REUSE_MEMORY_SIZE - 1];

    // Create kernel
    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(ctx.l0, "torch_benchmark_write_kernel.cl", "torch_benchmark_write_" + DataTypeHelper::toOpenclC(args.kernelDataType), kernel, module));

    // Warmup
    ze_group_count_t dispatch{1u, 1u, 1u};
    ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0<data_type>(ctx.cmdListImmediate_1, kernel, dispatch, d_reuse, 0, d_reuse_end, 0));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(ctx.cmdListImmediate_1, UINT64_MAX));

    // Benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<> offset_dist{0, 10};

    for (size_t i = 0; i < args.iterations; i++) {
        int offset1 = offset_dist(rng);
        int offset2 = -offset_dist(rng);

        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0<data_type>(ctx.cmdListImmediate_1, kernel, dispatch, d_reuse, offset1, d_reuse_end, offset2));
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize == 0)) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(ctx.cmdListImmediate_1, UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(ctx.cmdListImmediate_1, UINT64_MAX));

    zeMemFree(ctx.l0.context, d_reuse);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

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

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMemoryReuse> registerTestCase(run, Api::L0);
