/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_linear_kernel_size.h"
#include "kernel_submit_common.hpp"

using data_type = double;
const int WARMUP_ITERATIONS = 3;

static TestResult run_kernel(data_type *d_out, L0Context &l0_ctx, ze_kernel_handle_t kernel) {
    ze_group_count_t dispatch{static_cast<uint32_t>(1), 1, 1};
    ze_group_size_t groupSizes = {static_cast<uint32_t>(1), 1, 1};

    void *kernelArguments[1] = {&d_out};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel, dispatch, groupSizes,
                                                                          kernelArguments, nullptr, nullptr, 0, nullptr));

    return TestResult::Success;
}

static TestResult run(const KernelSubmitLinearKernelSizeArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    if (args.kernelSize != 32 &&
        args.kernelSize != 128 &&
        args.kernelSize != 512 &&
        args.kernelSize != 1024 &&
        args.kernelSize != 5120) {
        std::cerr << "Invalid kernel size: " << args.kernelSize << ". Allowed sizes are 32, 128, 512, 1024, 5120." << std::endl;
        return TestResult::Error;
    }

    L0Context l0_ctx{};

    // allocate device memory
    data_type *d_out = nullptr;
    l0_malloc_device<data_type>(l0_ctx.l0, 1, d_out);
    if (!d_out) {
        return TestResult::Error;
    }

    // create kernel
    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    const auto kernelName = "torch_benchmark_linear_kernel_size_" + std::to_string(args.kernelSize);
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, kernelName + ".cl", kernelName, kernel, module));
    // warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        ASSERT_TEST_RESULT_SUCCESS(run_kernel(d_out, l0_ctx, kernel));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));

    // benchmarking
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(run_kernel(d_out, l0_ctx, kernel));
        profiler.measureEnd();
        profiler.pushStats(statistics);

        // expect a wait here after a batch of submissions, if batch > 0
        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize) == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));

    // verify and clean up
    data_type host_result[1] = {0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(l0_ctx.cmdListImmediate_1, &host_result, d_out, sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));

    if (host_result[0] > (static_cast<data_type>(args.kernelSize) + 0.1) || host_result[0] < (static_cast<data_type>(args.kernelSize) - 0.1)) {
        std::cout << "Wrong checker value: " << host_result[0] << ", expected " << static_cast<data_type>(args.kernelSize) << std::endl;
        return TestResult::Error;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_out));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitLinearKernelSize> registerTestCase(run, Api::L0, true);
