/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_linear_kernel_size.h"

using data_type = double;

static TestResult run(const KernelSubmitLinearKernelSizeArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    if (args.kernelSize != 32 &&
        args.kernelSize != 128 &&
        args.kernelSize != 512 &&
        args.kernelSize != 1024 &&
        args.kernelSize != 5120) {
        std::cerr << "Invalid kernel size: " << args.kernelSize << ". Allowed sizes are 32, 128, 512, 1024, 5120." << std::endl;
        return TestResult::InvalidArgs;
    }

    // setup
    LevelZero l0{};
    CommandList cmd_list(l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc);
    DeviceMemory<data_type> d_out(l0, 1);

    // create kernel
    const auto kernelName = "linear_kernel_size_" + std::to_string(args.kernelSize);
    Kernel kernel(l0, "torch_benchmark_" + kernelName + ".cl", kernelName);
    const ze_group_count_t dispatch{static_cast<uint32_t>(1), 1, 1};
    const ze_group_size_t groupSizes{static_cast<uint32_t>(1), 1, 1};
    void *kernel_args[1] = {d_out.getAddress()};

    auto submit_kernel = [&]() -> TestResult {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list.get(), kernel.get(), dispatch, groupSizes,
                                                                              kernel_args, nullptr, nullptr, 0, nullptr));
        return TestResult::Success;
    };

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel());
        profiler.measureEnd();
        profiler.pushStats(statistics);

        // expect a wait here after a batch of submissions, if batch > 0
        if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize) == 0) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

    // verify result
    data_type host_result[1] = {0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list.get(), &host_result, d_out.getPtr(), sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

    if (std::abs(host_result[0] - static_cast<data_type>(args.kernelSize)) > static_cast<data_type>(epsilon)) {
        std::cout << "Wrong checker value: " << host_result[0] << ", expected " << static_cast<data_type>(args.kernelSize) << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitLinearKernelSize> registerTestCase(run, Api::L0, true);
