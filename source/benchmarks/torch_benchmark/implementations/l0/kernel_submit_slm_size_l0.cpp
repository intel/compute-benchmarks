/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_slm_size.h"

using data_type = float;

static TestResult compute_slm_num(int &slm_num, LevelZero &l0) {
    ze_device_compute_properties_t computeProps = {};
    computeProps.stype = ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
    zeDeviceGetComputeProperties(l0.device, &computeProps);
    const uint32_t max_slm_size = computeProps.maxSharedLocalMemory;

    if (slm_num < 0) {
        slm_num = max_slm_size / sizeof(data_type);
    }

    if (static_cast<uint32_t>(slm_num * sizeof(data_type)) > max_slm_size) {
        return TestResult::Error;
    }
    return TestResult::Success;
}

static TestResult run(const KernelSubmitSlmSizeArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    LevelZero l0{};
    CommandList cmd_list{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    DeviceMemory<data_type> out_buf{l0, 2};

    int slm_num = static_cast<int>(args.slmNum);
    ASSERT_TEST_RESULT_SUCCESS(compute_slm_num(slm_num, l0));

    // create kernel
    const std::string kernelName = "elementwise_slm";
    Kernel kernel{l0, "torch_benchmark_" + kernelName + ".cl", kernelName};
    const int wgs = std::min(slm_num, 1024);
    int slm_size = slm_num * sizeof(data_type);
    void *kernelArguments[3] = {out_buf.getAddress(), &slm_num, &slm_size};
    ze_group_size_t groupSizes = {static_cast<uint32_t>(wgs), 1, 1};
    ze_group_count_t dispatch{1, 1, 1};

    auto submit_kernel = [&]() -> TestResult {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list.get(), kernel.get(), dispatch, groupSizes,
                                                                              kernelArguments, nullptr, nullptr, 0, nullptr));
        return TestResult::Success;
    };

    // benchmark
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        for (size_t j = 0; j < args.kernelBatchSize; ++j) {
            ASSERT_TEST_RESULT_SUCCESS(submit_kernel());
        }

        if (!args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

        if (args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
    }

    // verify result
    data_type host_result[2];
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list.get(), &host_result, out_buf.getPtr(), 2 * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list.get(), UINT64_MAX));

    if (host_result[1] > 13.1 || host_result[1] < 12.9) {
        std::cerr << "Wrong checker value: " << host_result[1] << ", expected 13.0, result value:" << host_result[0] << std::endl;
        return TestResult::Error;
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitSlmSize> registerTestCase(run, Api::L0, true);
