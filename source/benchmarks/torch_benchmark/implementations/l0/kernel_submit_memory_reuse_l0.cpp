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
    // setup
    LevelZero l0{};
    CommandList cmd_list{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};

    DeviceMemory<data_type> d_reuse{l0, REUSE_MEMORY_SIZE / sizeof(data_type)};
    auto d_reuse_addr = d_reuse.getAddress();
    data_type *d_reuse_end = &d_reuse.getPtr()[REUSE_MEMORY_SIZE / sizeof(data_type) - 1];

    // create kernel
    const std::string kernel_name = "write_" + DataTypeHelper::toOpenclC(args.kernelDataType);
    const auto kernel_file = "torch_benchmark_write.cl";
    Kernel kernel{l0, kernel_file, kernel_name};
    const ze_group_count_t dispatch{1u, 1u, 1u};
    const ze_group_size_t groupSizes{1u, 1u, 1u};

    auto submit_kernel = [&](int offset_1, int offset_2) -> TestResult {
        void *kernel_args[4] = {d_reuse_addr, static_cast<void *>(&offset_1), &d_reuse_end, static_cast<void *>(&offset_2)};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list.get(), kernel.get(), dispatch, groupSizes,
                                                                              kernel_args, nullptr, nullptr, 0, nullptr));
        return TestResult::Success;
    };

    // benchmark
    std::mt19937 rng(42);
    std::uniform_int_distribution<> offset_dist{0, 10};

    for (size_t i = 0; i < args.iterations; i++) {
        int offset1 = offset_dist(rng);
        int offset2 = -offset_dist(rng);

        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(offset1, offset2));
        profiler.measureEnd();
        profiler.pushStats(statistics);

        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize == 0)) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

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
